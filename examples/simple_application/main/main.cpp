#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <core/Logger.h>

extern "C" void app_main() {
    esp_logi(mon, "\tfree-heap: %lu", esp_get_free_heap_size());
    esp_logi(mon, "\tstack-watermark: %d", uxTaskGetStackHighWaterMark(nullptr));

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
