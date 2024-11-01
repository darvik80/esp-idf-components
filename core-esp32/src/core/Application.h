//
// Created by Ivan Kishchenko on 19/08/2023.
//

#pragma once

#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>

#include "Registry.h"

#include <algorithm>
#include <esp_app_desc.h>

template<typename T>
class Application : public std::enable_shared_from_this<T> {
protected:
    virtual void userSetup() {}

public:
    Registry &getRegistry() {
        return Registry::getInstance();
    }

    virtual void setup() {
        auto appDesc = esp_app_get_description();
        esp_logi(app, "idf-ver: %s", appDesc->idf_ver);
        esp_logi(app, "app-name: %s", appDesc->project_name);
        esp_logi(app, "app-ver: %s", appDesc->version);
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

#ifndef CONFIG_IDF_TARGET_LINUX
        esp_vfs_spiffs_conf_t conf = {
                .base_path = "/spiffs",
                .partition_label = "storage",
                .max_files = 5,
                .format_if_mount_failed = true,
        };
        if (auto err = esp_vfs_spiffs_register(&conf); err != ESP_OK) {
            esp_loge(app, "Failed to init flash: %d:%s", err, esp_err_to_name(err));
        }
#endif

        esp_event_loop_create_default();

        userSetup();
        std::sort(getRegistry().getServices().begin(), getRegistry().getServices().end(), [](auto f, auto s) {
            return f->getServiceId() < s->getServiceId();
        });

#ifndef CONFIG_IDF_TARGET_LINUX
        getRegistry().getPropsLoader().load("/spiffs/config.json");
#else
        getRegistry().getPropsLoader().load("config.json");
#endif

        for (auto service: getRegistry().getServices()) {
            if (service) {
                service->setup();
                esp_logd(app, "Setup: 0x%04x:%s", service->getServiceId(), service->getServiceName().data());
            }
        }
    }

    virtual void destroy() {
#ifndef CONFIG_IDF_TARGET_LINUX
        ESP_ERROR_CHECK(esp_vfs_spiffs_unregister("storage"));
#endif

        ESP_ERROR_CHECK(esp_event_loop_delete_default());
    }

    virtual ~Application() = default;
};
