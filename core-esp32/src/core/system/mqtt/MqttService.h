//
// Created by Ivan Kishchenko on 10/09/2023.
//

#pragma once

#include <esp_event.h>
#include <mqtt_client.h>
#include <set>
#include <map>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include "core/system/System.h"

#include "MqttBrokersList.h"
#include "MqttProperties.h"

#include <core/StateMachine.h>

ESP_EVENT_DECLARE_BASE(MQTT_INTERNAL_EVENT);

namespace mqtt {
    enum Event {
        Wifi_EvtDisconnected,
        Wifi_EvtConnected,
        Mqtt_EvtConnecting,
        Mqtt_EvtConnected,
        Mqtt_EvtDisconnected,
    };

    struct WifiDisconnectedState : State<
            StateEvent<Wifi_EvtConnected>, struct WifiConnectedState> {
    };

    struct WifiConnectedState : State<
            StateEvent<Mqtt_EvtConnecting>, struct ConnectingState,
            StateEvent<Wifi_EvtDisconnected>, struct WifiDisconnectedState> {
    };

    struct ConnectingState : State<
            StateEvent<Wifi_EvtDisconnected>, struct WifiDisconnectedState,
            StateEvent<Mqtt_EvtConnected>, struct ConnectedState> {
    };

    struct ConnectedState : State<
            StateEvent<Mqtt_EvtDisconnected>, struct DisconnectedState,
            StateEvent<Wifi_EvtDisconnected>, struct WifiDisconnectedState> {
    };

    struct DisconnectedState : State<
            StateEvent<Mqtt_EvtConnecting>, struct ConnectingState,
            StateEvent<Wifi_EvtDisconnected>, struct WifiDisconnectedState> {
    };
}

typedef std::function<void(const cJSON *json)> MqttJsonHandler;

enum mqtt_sub_type_t {
    MQTT_SUB_RELATIVE,
    MQTT_SUB_BROADCAST,
    MQTT_SUB_ABSOLUTE,
};

struct mqtt_sub_info {
    mqtt_sub_type_t type;
    MqttJsonHandler handler;
};

class MqttService
        : public TService<MqttService, Service_Sys_Mqtt, Sys_Core>,
          public TPropertiesConsumer<MqttService, MqttProperties>,
          public TMessageSubscriber<MqttService, SystemEventChanged>,
          public StateMachine<MqttService, mqtt::WifiDisconnectedState, mqtt::WifiConnectedState, mqtt::ConnectingState, mqtt::ConnectedState, mqtt::DisconnectedState> {

    EventGroupHandle_t _eventGroup;
    esp_mqtt_client_handle_t _client{nullptr};

    std::string _prefix;
    std::string _broadcast;
    std::map<std::string, mqtt_sub_info> _handlers;

    MqttBrokersList _balancer;
    EspTimer _reconTimer;

    enum State {
        S_Unknown,
        S_Wifi_Connected,
        S_Mqtt_Connected,
        S_Mqtt_WaitConnection,
    };
    State _state{S_Unknown};
private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        auto *self = static_cast<MqttService *>(arg);
        self->eventHandler(event_base, event_id, event_data);
    }

    static bool compareTopics(std::string_view topic, std::string_view origin);

    void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void eventHandlerConnect(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void eventHandlerDisconnect(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void eventHandlerData(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void eventHandlerError(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void createConnection(MqttBrokersList::BrokerInfo &info);

    void destroyConnection();
public:

    explicit MqttService(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "mqtt";
    }

    void handle(const SystemEventChanged &msg);

    void apply(const MqttProperties &props);

    void setup() override;

    template<typename T>
    void addJsonHandler(std::string_view topic, mqtt_sub_type_t type) {
        _handlers.emplace(
                topic,
                mqtt_sub_info{
                        .type = type,
                        .handler = [this](const cJSON *json) {
                            T msg;
                            fromJson(json, msg);
                            getBus().post(msg);
                        }
                }
        );
    }

    template<typename T>
    void addJsonProcessor(std::string_view topic) {
        getBus().subscribe<T>([this, topic](const T &msg) {
            if (xEventGroupWaitBits(_eventGroup, BIT0, pdFALSE, pdFALSE, 0)) {
                auto json = cJSON_CreateObject();
                toJson(json, msg);
                auto str = cJSON_PrintUnformatted(json);
                cJSON_Delete(json);

                std::string fullTopic = _prefix + topic.data();
                esp_logd(mqtt, "publish %s", fullTopic.c_str());
                esp_mqtt_client_publish(_client, fullTopic.c_str(), str, (int) strlen(str), 0, false);
                cJSON_free(str);
            } else {
                esp_logw(mqtt, "client not ready");
            }
        });
    }

    ~MqttService() override;

public:
    void onStateChanged(const TransitionTo<mqtt::WifiConnectedState> &);

    void onStateChanged(const TransitionTo<mqtt::WifiDisconnectedState> &);

    void onStateChanged(const TransitionTo<mqtt::DisconnectedState> &);

    void onStateChanged(const TransitionTo<mqtt::ConnectingState> &);

    void onStateChanged(const TransitionTo<mqtt::ConnectedState> &);
};
