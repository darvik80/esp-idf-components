//
// Created by Ivan Kishchenko on 21/09/2023.
//

#include "WifiService.h"

WifiService::WifiService(Registry &registry) : TService(registry) {
    registry.getPropsLoader().addReader("wifi", defaultPropertiesReader<WifiProperties>);
    registry.getPropsLoader().addConsumer(this);
}

void WifiService::eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_scan_start(nullptr, false));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        auto *event = static_cast<wifi_event_sta_connected_t *>(event_data);
        StateMachine::handle(StateEvent<wifi::Wifi_EvtApConnected>{}, *event);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        auto *event = static_cast<wifi_event_sta_disconnected_t *>(event_data);
        StateMachine::handle(StateEvent<wifi::Wifi_EvtDisconnected>{}, *event);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto *event = (ip_event_got_ip_t *) event_data;
        StateMachine::handle(StateEvent<wifi::Wifi_EvtConnected>{}, *event);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        StateMachine::handle(StateEvent<wifi::Wifi_EvtScanDone>{});
    } else {
        esp_logd(wifi, "wifi event: %lu", event_id);
    }
}

void WifiService::handle(const Command &cmd) {
    if (strcmp(cmd.cmd, "wifi") == 0) {
        if (strcmp(cmd.params, "scan") == 0) {
            if (!std::get_if<wifi::ScanningState *>(&getCurrentState())) {
                StateMachine::handle(StateEvent<wifi::Wifi_ReqScan>{});
            } else {
                esp_logi(mqtt, LOG_COLOR(LOG_COLOR_RED) "... scan in progress ...");
            }
        } else if (strcmp(cmd.params, "ip") == 0) {
            if (std::get_if<wifi::ConnectedState *>(&getCurrentState())) {
                esp_netif_ip_info_t ip_info;
                esp_netif_get_ip_info(_netif, &ip_info);
                esp_logi(
                        wifi,
                        "ip result:\r\n\tip: " IPSTR "\r\n\tmask: " IPSTR "\r\n\tgw" IPSTR,
                        IP2STR(&ip_info.ip),
                        IP2STR(&ip_info.netmask),
                        IP2STR(&ip_info.gw)
                );
            } else {
                esp_logi(wifi, "not connected");
            }
        }
    }
}

void WifiService::apply(const WifiProperties &props) {
    esp_logi(wifi, "apply wifi props: %d", props.ap.size());
    for (const auto &it: props.ap) {
        esp_logi(wifi, "\tssid: %s", it.ssid.c_str());
    }
    _props = props;
}

void WifiService::setup()
{
    if (!_props.ap.empty())
    {
        _props.ap.push_back({
            .ssid = CONFIG_CORE_WIFI_DEFAULT_SSID,
            .password = CONFIG_CORE_WIFI_DEFAULT_PASSWORD,
        });
    }
    ESP_ERROR_CHECK(esp_netif_init());
    _netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            eventHandler,
            this,
            nullptr
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            eventHandler,
            this,
            nullptr
    ));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());
}

void WifiService::onStateChanged(const TransitionTo<wifi::DisconnectedState> &, const wifi_event_sta_disconnected_t& event) {
    esp_logd(wifi, "onStateChanged::TransitionTo<wifi::DisconnectedState>");
    if (!std::get_if<wifi::ConnectingState *>(&getPrevState())) {
        esp_logi(wifi, "Disconnected from AP: " LOG_COLOR(LOG_COLOR_CYAN) "%s", event.ssid);
        getBus().post(SystemEventChanged{.status = SystemStatus::Wifi_Disconnected});
    }
    StateMachine::handle(StateEvent<wifi::Wifi_ReqConnect>{});
}

void WifiService::onStateChanged(const TransitionTo<wifi::ConnectingState> &) {
    esp_logd(wifi, "onStateChanged::TransitionTo<wifi::ConnectingState>");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void WifiService::onStateChanged(const TransitionTo<wifi::ApConnectedState> &, const wifi_event_sta_connected_t &event) {
    esp_logd(wifi, "onStateChanged::TransitionTo<wifi::ApConnectedState>");
    esp_logi(wifi, "Connected to AP: " LOG_COLOR(LOG_COLOR_CYAN) "%s", event.ssid);
}

void WifiService::onStateChanged(const TransitionTo<wifi::ConnectedState> &, const ip_event_got_ip_t &event) {
    esp_logd(wifi, "onStateChanged::TransitionTo<wifi::ConnectedState>");
    esp_logi(wifi, "Connected, got ip: " LOG_COLOR(LOG_COLOR_CYAN) IPSTR, IP2STR(&event.ip_info.ip));
    getBus().post(SystemEventChanged{.status = SystemStatus::Wifi_Connected});
}

void WifiService::onStateChanged(const TransitionTo<wifi::ScanningState> &) {
    esp_logd(wifi, "onStateChanged::TransitionTo<wifi::ScanningState>");
    if (std::get_if<wifi::ConnectedState *>(&getPrevState()) || std::get_if<wifi::ApConnectedState *>(&getPrevState())) {
        wifi_ap_record_t wifi_ap_record;
        esp_wifi_sta_get_ap_info(&wifi_ap_record);
        esp_logi(wifi, "Force disconnect, from AP: " LOG_COLOR(LOG_COLOR_CYAN) "%s", wifi_ap_record.ssid);
        esp_wifi_disconnect();
        getBus().post(SystemEventChanged{.status = SystemStatus::Wifi_Disconnected});
    }

    wifi_scan_config_t scan{
            .show_hidden = false,
            .scan_type = WIFI_SCAN_TYPE_ACTIVE,
            .scan_time {
                    .active {
                            .min = 1000,
                            .max = 10000,
                    },
                    .passive = 10000
            }
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan, false));
}

void WifiService::onStateChanged(const TransitionTo<wifi::ScanCompletedState> &) {
    esp_logd(wifi, "onStateChanged::TransitionTo<wifi::ScanCompletedState>");
    uint16_t appsNumber{0};
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&appsNumber));
    esp_logi(wifi, "scan done, found AP: %d", appsNumber);
    if (appsNumber) {
        auto *list = (wifi_ap_record_t *) malloc(sizeof(wifi_ap_record_t) * appsNumber);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&appsNumber, list));

        auto it = std::end(_props.ap);
        for (uint16_t idx = 0; idx < appsNumber; idx++) {
            esp_logi(wifi, "\tssid: %s, rssi: %d", list[idx].ssid, list[idx].rssi);
            if (it == std::end(_props.ap)) {
                it = std::find_if(_props.ap.begin(), _props.ap.end(), [&](const auto &ap) {
                    return ap.ssid == (const char *) list[idx].ssid;
                });
            }
        }
        free(list);

        if (it != std::end(_props.ap)) {
            esp_logi(wifi, "trying connect to AP: %s", it->ssid.c_str());
            wifi_config_t cfg{};
            memcpy(cfg.sta.ssid, it->ssid.c_str(), it->ssid.size());
            memcpy(cfg.sta.password, it->password.c_str(), it->password.size());
            cfg.sta.threshold.authmode = it->password.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
            StateMachine::handle(StateEvent<wifi::Wifi_ReqConnect>{});
        } else {
            esp_logw(wifi, "not found configured AP - re-scan");
            esp_wifi_scan_start(nullptr, false);
        }
    } else {
        esp_wifi_scan_start(nullptr, false);
    }
}

std::string getWifiMacAddress() {
    uint8_t mac_addr[6];
    char mac_addr_str[19];
    esp_wifi_get_mac(static_cast<wifi_interface_t>(ESP_IF_WIFI_STA), mac_addr);
    snprintf(mac_addr_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);

    return mac_addr_str;
}