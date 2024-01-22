//
// Created by Ivan Kishchenko on 21/09/2023.
//

#if defined(CONFIG_ESP32_WIFI_ENABLED)
#include <esp_wifi.h>
#endif
#include "TelemetryService.h"

#include <esp_heap_caps.h>

TelemetryService::TelemetryService(Registry&registry) : TService(registry) {
    _timer.fire<SysTid_Telemetry>(10000, true);

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
    temperature_sensor_config_t temp_sensor = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor, &_temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_enable(_temp_handle));
#endif
}

void TelemetryService::onEvent(const TimerEvent<SysTid_Telemetry>&msg) {
    const size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    const size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

    Telemetry telemetry{
        .freeHeap = freeHeap,
        .usedMemPercent = ((double)(totalHeap - freeHeap) / (double)totalHeap) * 100,
        .stackWatermark = uxTaskGetStackHighWaterMark(nullptr),
    };
#ifdef  CONFIG_ESP32_WIFI_ENABLED
    esp_wifi_sta_get_rssi(&telemetry.wifiRssi);
#endif

    esp_logi(mon, "telemetry:");
    esp_logi(mon, "\tfree-heap: %zu", telemetry.freeHeap);
    esp_logi(mon, "\tused-mem-percent: %f", telemetry.usedMemPercent);
    esp_logi(mon, "\tstack-watermark: %zu", telemetry.stackWatermark);
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
    float temp;
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(_temp_handle, &temp));
    esp_logi(mon, "\ttemperature: %f °C", temp);
    telemetry.temperature = temp;
#endif

    getBus().send(telemetry);
}

TelemetryService::~TelemetryService() {
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
    ESP_ERROR_CHECK(temperature_sensor_disable(_temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_uninstall(_temp_handle));
#endif
}
