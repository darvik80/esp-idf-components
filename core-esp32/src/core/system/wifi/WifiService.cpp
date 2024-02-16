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
        esp_logi(wifi, "started");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        getBus().post(SystemEventChanged{.status = SystemStatus::Wifi_Disconnected});
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        getBus().post(SystemEventChanged{.status = SystemStatus::Wifi_Connected,});
        auto *event = (ip_event_got_ip_t *) event_data;
        esp_logi(wifi, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        esp_logi(wifi, "scan done...");
        getBus().post(SystemEventChanged{.status = SystemStatus::Wifi_ScanDone,});
    } else {
        esp_logi(wifi, "wifi event: %lu", event_id);
    }
}

void WifiService::handle(const Command &cmd) {
    if (strcmp(cmd.cmd, "wifi") == 0) {
        if (strcmp(cmd.params, "scan") == 0) {
            esp_wifi_disconnect();
            esp_wifi_scan_start(nullptr, false);
        } else if (strcmp(cmd.params, "ip") == 0) {
            esp_netif_ip_info_t ip_info;
            esp_netif_get_ip_info(_netif, &ip_info);
            esp_logi(
                    wifi,
                    "ip result:\r\n\tip: " IPSTR "\r\n\tmask: " IPSTR "\r\n\tgw" IPSTR,
                    IP2STR(&ip_info.ip),
                    IP2STR(&ip_info.netmask),
                    IP2STR(&ip_info.gw)
            );
        }
    }
}

void WifiService::handle(const SystemEventChanged &msg) {
    if (msg.status == SystemStatus::Wifi_ScanDone) {
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
                ESP_ERROR_CHECK(esp_wifi_connect());
            } else {
                esp_logw(wifi, "not found configured AP - re-scan");
                esp_wifi_scan_start(nullptr, false);
            }
        } else {
            esp_wifi_scan_start(nullptr, false);
        }
    }
}

void WifiService::apply(const WifiProperties &props) {
    esp_logi(wifi, "apply wifi props: %d", props.ap.size());
    for (const auto &it: props.ap) {
        esp_logi(wifi, "\tssid: %s", it.ssid.c_str());
    }
    _props = props;

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
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_start(nullptr, false));
}
