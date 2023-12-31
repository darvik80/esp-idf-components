//
// Created by Ivan Kishchenko on 19/08/2023.
//

#pragma once

#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>

#include "Registry.h"

template<typename T>
class Application : public std::enable_shared_from_this<T>{
    Registry _registry;

protected:
    virtual void userSetup() {}

public:
    Registry &getRegistry() {
        return _registry;
    }

    virtual void setup() {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        esp_event_loop_create_default();

        esp_vfs_spiffs_conf_t conf = {
                .base_path = "/spiffs",
                .partition_label = "storage",
                .max_files = 5,
                .format_if_mount_failed = true,
        };
        ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

        userSetup();
        std::sort(getRegistry().getServices().begin(), getRegistry().getServices().end(), [](auto f, auto s) {
            return f->getServiceId() < s->getServiceId();
        });

        getRegistry().getPropsLoader().load("/spiffs/config.json");
        for (auto service: getRegistry().getServices()) {
            if (service) {
                service->setup();
                esp_logd(app, "Setup: %d", service->getServiceId());
            }
        }
    }

    virtual void destroy() {
        ESP_ERROR_CHECK(esp_vfs_spiffs_unregister("storage"));
        ESP_ERROR_CHECK(esp_event_loop_delete_default());
    }

    virtual ~Application() = default;
};
