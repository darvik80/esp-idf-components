//
// Created by Ivan Kishchenko on 10/09/2023.
//

#include "MqttService.h"
#include "core/system/wifi/WifiService.h"

ESP_EVENT_DEFINE_BASE(MQTT_INTERNAL_EVENT);

bool MqttService::compareTopics(std::string_view topic, std::string_view origin) {
    enum class State {
        None,
        Slash,
    } state = State::None;
    auto it = topic.begin(), oit = origin.begin();

    while (it != topic.end() && oit != origin.end()) {
        switch (state) {
            case State::Slash:
                if (*it == '#') {
                    if (*oit == '/') return false;
                    if (++oit == origin.end()) ++it;
                    continue;
                } else if (*it == '+') {
                    if (*oit != '/') {
                        ++oit;
                        continue;
                    } else ++it;
                } else {
                    if (*it == '/') ++oit;
                    state = State::None;
                }
                break;
            case State::None:
            default:
                if (*it == '/') {
                    if (*oit != '/') return false;
                    state = State::Slash;
                } else if (*it == '+' || *it == '#') {
                    return false;
                }
                if (*it != *oit) return false;
        }

        ++it, ++oit;
    }

    if (it != topic.end() && *it == '+') {
        ++it;
    }

    return it == topic.end() && oit == origin.end();
}


MqttService::MqttService(Registry &registry) : TService(registry) {
    _eventGroup = xEventGroupCreate();
    registry.getPropsLoader().addReader("mqtt", defaultPropertiesReader<MqttProperties>);
    registry.getPropsLoader().addConsumer(this);

    ESP_ERROR_CHECK(esp_event_handler_register(MQTT_INTERNAL_EVENT, ESP_EVENT_ANY_ID, eventHandler, this));
}

static void log_error_if_nonzero(const char *message, int error_code) {
    if (error_code != 0) {
        esp_logi(mqtt, "Last error %s: 0x%x", message, error_code);
    }
}

void MqttService::eventHandlerConnect(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    for (const auto &it: _handlers) {
        std::string fullPath;
        switch (it.second.type) {
            case MQTT_SUB_RELATIVE:
                fullPath = _prefix;
                break;
            case MQTT_SUB_BROADCAST:
                fullPath = _broadcast;
            default:
                break;
        }
        fullPath.append(it.first);

        int msg_id = esp_mqtt_client_subscribe(_client, fullPath.c_str(), 0);
        esp_logi(mqtt, "sub successful, topic: %s, msg_id: %d", fullPath.c_str(), msg_id);
    }
    ESP_ERROR_CHECK(esp_event_post(MQTT_INTERNAL_EVENT, MQTT_EVENT_CONNECTED, nullptr, 0, portMAX_DELAY));
}

void MqttService::eventHandlerDisconnect(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_ERROR_CHECK(esp_event_post(MQTT_INTERNAL_EVENT, MQTT_EVENT_DISCONNECTED, nullptr, 0, portMAX_DELAY));
}

void MqttService::eventHandlerData(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);

    static std::string segment;

//    // prepare topic
    std::string_view topic(event->topic, event->topic_len);
    // prepare payload
    std::string_view message(event->data, event->data_len);
    // process segment data
    if (event->data_len < event->total_data_len) {
        if (event->current_data_offset == 0) {
            segment.clear();
        }
        segment.append(event->data, event->data_len);
        if (segment.size() < event->total_data_len) {
            return;
        }

        message = segment;
    }

    for (const auto &it: _handlers) {
        std::string fullPath;
        switch (it.second.type) {
            case MQTT_SUB_RELATIVE:
                fullPath = _prefix;
                break;
            case MQTT_SUB_BROADCAST:
                fullPath = _broadcast;
            default:
                break;
        }
        fullPath.append(it.first);
        if (compareTopics(fullPath, topic)) {
            auto json = cJSON_ParseWithLength(message.data(), message.size());
            if (json) {
                it.second.handler(json);
                cJSON_Delete(json);
            }
        }
    }
}

void MqttService::eventHandlerError(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);

    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno",
                             event->error_handle->esp_transport_sock_errno);
        esp_logd(mqtt, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
    }
}


void MqttService::eventHandler(esp_event_base_t base, int32_t event_id, void *event_data) {
    if (base == MQTT_INTERNAL_EVENT) {
        switch (event_id) {
            case MQTT_EVENT_CONNECTED:
                StateMachine::handle(StateEvent<mqtt::Mqtt_EvtConnected>{});
                break;
            case MQTT_EVENT_DISCONNECTED:
                StateMachine::handle(StateEvent<mqtt::Mqtt_EvtDisconnected>{});
                break;
            default:
                break;
        }
    } else { // process messages in MQTT Task
        switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
            case MQTT_EVENT_BEFORE_CONNECT:
                break;
            case MQTT_EVENT_CONNECTED:
                xEventGroupSetBits(_eventGroup, BIT0);
                eventHandlerConnect(base, event_id, event_data);
                break;
            case MQTT_EVENT_DISCONNECTED:
                xEventGroupSetBits(_eventGroup, BIT1);
                eventHandlerDisconnect(base, event_id, event_data);
                break;
            case MQTT_EVENT_DATA:
                eventHandlerData(base, event_id, event_data);
                break;
            case MQTT_EVENT_ERROR:
                eventHandlerError(base, event_id, event_data);
                break;
            default:
                esp_logd(mqtt, "event: %ld", event_id);
                break;
        }
    }
}

void MqttService::createConnection(MqttBrokersList::BrokerInfo &broker) {
    destroyConnection();
    std::string clientId = broker.clientId.empty() ? broker.deviceName + "-" + getWifiMacAddress(): broker.clientId;
    esp_mqtt_client_config_t mqtt_cfg = {
            .broker{
                    .address {
                            .uri = broker.uri.data(),
                    }
            },
            .credentials {
                    .username = broker.username.c_str(),
                    .client_id = clientId.c_str(),
                    .authentication {
                            .password = broker.password.c_str()
                    }
            },
            .network {
                    .timeout_ms = 3000,
                    .disable_auto_reconnect = false,
            }
    };

    if (!broker.caCert.empty()) {
        mqtt_cfg.broker.verification.certificate = broker.caCert.c_str();
    }
    if (!broker.clientCert.empty()) {
        mqtt_cfg.credentials.authentication.certificate = broker.clientCert.c_str();
    }
    if (!broker.clientKey.empty()) {
        mqtt_cfg.credentials.authentication.key = broker.clientKey.c_str();
    }
    _client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, eventHandler, this);
    _prefix = "/" + broker.productName + "/" + broker.deviceName;
    _broadcast = "/" + broker.productName;

    ESP_ERROR_CHECK(esp_mqtt_client_start(_client));
}

void MqttService::destroyConnection() {
    if (_client) {
        esp_mqtt_client_unregister_event(_client, MQTT_EVENT_ANY, eventHandler);
        esp_mqtt_client_destroy(_client);
        _client = nullptr;
    }
}

void MqttService::handle(const SystemEventChanged &msg) {
    switch (msg.status) {
        case SystemStatus::Wifi_Connected:
            esp_logd(mqtt, "[onEvent::Wifi_Connected]");
            StateMachine::handle(StateEvent<mqtt::Wifi_EvtConnected>{});
            break;
        case SystemStatus::Wifi_Disconnected:
            esp_logd(mqtt, "[onEvent::Wifi_Disconnected]");
            StateMachine::handle(StateEvent<mqtt::Wifi_EvtDisconnected>{});
            break;
        case SystemStatus::Mqtt_Connected:
            esp_logd(mqtt, "[onEvent::Mqtt_Connected]");
            StateMachine::handle(StateEvent<mqtt::Mqtt_EvtConnected>{});
            break;
        case SystemStatus::Mqtt_Disconnected:
            StateMachine::handle(StateEvent<mqtt::Mqtt_EvtDisconnected>{});
            break;
        default:
            break;
    }
}

void MqttService::apply(const MqttProperties &props) {
    _balancer.apply(props);
}

void MqttService::setup()
{
   if (!_balancer.size())
   {
       MqttProperties props;
       props.brokers.push_back({
           .type = CONFIG_CORE_MQTT_DEFAULT_TYPE,
           .uri = CONFIG_CORE_MQTT_DEFAULT_URI,
           .username = CONFIG_CORE_MQTT_DEFAULT_USERNAME,
           .password = CONFIG_CORE_MQTT_DEFAULT_PASSWORD,
           .productName = CONFIG_CORE_MQTT_DEFAULT_PRODUCT_NAME,
           .deviceName = CONFIG_CORE_MQTT_DEFAULT_DEVICE_NAME,
       });
       _balancer.apply(props);
   }
}

MqttService::~MqttService() {
    vEventGroupDelete(_eventGroup);
}

void MqttService::onStateChanged(const TransitionTo<mqtt::WifiConnectedState> &) {
    esp_logd(mqtt, LOG_COLOR(LOG_COLOR_BROWN) "[onStateChanged::WifiConnectedState]");
    auto [broker, update] = _balancer.getNextBroker();
    esp_logi(mqtt, "Connecting...: %s", broker.uri.c_str());
    createConnection(broker);
    StateMachine::handle(StateEvent<mqtt::Mqtt_EvtConnecting>{});
}

void MqttService::onStateChanged(const TransitionTo<mqtt::WifiDisconnectedState> &) {
    esp_logd(mqtt, LOG_COLOR(LOG_COLOR_BROWN) "[onStateChanged::WifiDisconnectedState]");
    if (std::get_if<mqtt::ConnectedState *>(&getPrevState())) {
        destroyConnection();
        getBus().post(SystemEventChanged{.status = SystemStatus::Mqtt_Disconnected});
    }
}

void MqttService::onStateChanged(const TransitionTo<mqtt::ConnectingState> &) {
    esp_logd(mqtt, LOG_COLOR(LOG_COLOR_BROWN) "[onStateChanged::ConnectingState]");
}

void MqttService::onStateChanged(const TransitionTo<mqtt::ConnectedState> &) {
    esp_logd(mqtt, LOG_COLOR(LOG_COLOR_BROWN) "[onStateChanged::ConnectedState]");
    getBus().post(SystemEventChanged{.status = SystemStatus::Mqtt_Connected,});
}

void MqttService::onStateChanged(const TransitionTo<mqtt::DisconnectedState> &) {
    esp_logd(mqtt, LOG_COLOR(LOG_COLOR_BROWN) "[onStateChanged::DisconnectedState]");
    getBus().post(SystemEventChanged{.status = SystemStatus::Mqtt_Disconnected});
    StateMachine::handle(StateEvent<mqtt::Mqtt_EvtConnecting>{});
}
