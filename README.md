# Darvik esp-idf framework

## core-esp32
Small framework for quick build your esp32 application

### Example

#### data/config.json
```json
{
  "wifi": [
    {
      "ssid": "ssid-ap",
      "password": "ssid-pass"
    },
    {
      "ssid": "alibaba-guest",
      "password": ""
    }
  ],
  "mqtt": {
    "type": "local",
    "uri": "mqtt://media-vn.local",
    "username": "mqtt",
    "password": "mqtt",
    "device-name": "magic-lamp",
    "product-name": "darvik-home"
  }
}
```

#### main/main.cpp
```cpp
class SimpleApplication
        : public Application<SimpleApplication>,
          public TEventSubscriber<SimpleApplication, SystemEventChanged> {
public:
    void userSetup() override {
        getRegistry().create<TelemetryService>();
        getRegistry().create<WifiService>();
        auto &mqtt = getRegistry().create<MqttService>();
        mqtt.addJsonProcessor<Telemetry>("/telemetry");
    }

    void onEvent(const SystemEventChanged &msg) {
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

    app->process();

    app->destroy();
}
```

## bluetooth scanner 
Component for support barcode scanners via spp, hid protocols


# TODO
* Support setup via 2 steps
    * Setup basic objects: wifi, mqtt
    * Continue setup after receive configuration from cloud