//
// Created by Ivan Kishchenko on 29/4/24.
//

#include "OtaService.h"

#include <esp_http_client.h>
#include <esp_https_ota.h>

static esp_err_t eventHandler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            esp_logi(ota, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            esp_logi(ota, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            esp_logi(ota, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            esp_logi(ota, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            esp_logd(ota, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            esp_logi(ota, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            esp_logi(ota, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            esp_logi(ota, "HTTP_EVENT_REDIRECT");
            break;
    }

    return ESP_OK;
}

void OtaService::setup() {
}

void OtaService::handle(const OtaUpdate& event) {
    auto appDesc = esp_app_get_description();
    if (strncmp(appDesc->version, event.version, 32) == 0) {
        return;
    }
    esp_logi(app, "cur-ver: %s, ota-ver: %s", appDesc->version, event.version);

    _task = FreeRTOSTask::submit([this]() {
        esp_http_client_config_t clientConfig = {
                .url = "https://darvik.synology.me/magic-lamp.bin",
                .auth_type = HTTP_AUTH_TYPE_NONE,
                //.cert_pem = (char *) (ali_cloud_cert_start),
                //.timeout_ms = 180000,
                .event_handler = eventHandler,
                .transport_type = HTTP_TRANSPORT_OVER_TCP,
                .skip_cert_common_name_check = false,
                .keep_alive_enable = true,

        };

        esp_https_ota_config_t config = {
                .http_config = &clientConfig
        };

        esp_err_t ret = esp_https_ota(&config);
        if (ret == ESP_OK) {
            esp_logi(ota, "Firmware upgrade successfully, restarting...");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            esp_logi(ota, "Firmware upgrade failed, %s", esp_err_to_name(ret));
        }
    }, "task-ota", 4096);
}