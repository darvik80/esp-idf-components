#include <core/Core.h>
#include "core/system/wifi/WifiService.h"
#include <core/system/mqtt/MqttService.h>
#include <core/system/telemetry/TelemetryService.h>

#include <sntp/SNTPService.h>

class SimpleApplication
        : public Application<SimpleApplication>,
          public TMessageSubscriber<SimpleApplication, SystemEventChanged> {
public:
    void userSetup() override {
        getRegistry().getEventBus().subscribe(shared_from_this());

        getRegistry().create<TelemetryService>();
        getRegistry().create<WifiService>();
        auto &mqtt = getRegistry().create<MqttService>();
        mqtt.addJsonProcessor<Telemetry>("/telemetry");

        getRegistry().create<SNTPService>();
    }

    void handle(const SystemEventChanged &msg) {
        switch (msg.status) {
            case SystemStatus::Wifi_Connected:
                esp_logi(app, "wifi-connected");
                break;
            case SystemStatus::Wifi_Disconnected:
                esp_logi(app, "wifi-disconnected");
                break;
            case SystemStatus::Mqtt_Connected:
                esp_logi(app, "mqtt-connected");
                break;
            case SystemStatus::Mqtt_Disconnected:
                esp_logi(app, "mqtt-disconnected");
                break;
            default:
                esp_logi(app, "some system event: %d", (int)msg.status);
                break;
        }
    }
};

extern "C" void app_main() {
    esp_logi(mon, "\tfree-heap: %lu", esp_get_free_heap_size());
    esp_logi(mon, "\tstack-watermark: %d", uxTaskGetStackHighWaterMark(nullptr));

    auto app = std::make_shared<SimpleApplication>();
    app->setup();
}
