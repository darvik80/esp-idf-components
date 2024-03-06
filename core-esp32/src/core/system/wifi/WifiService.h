//
// Created by Ivan Kishchenko on 21/09/2023.
//

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>

#include "core/system/System.h"
#include "WifiProperties.h"

class WifiService
        : public TService<WifiService, Service_Sys_Wifi, Sys_Core>,
          public TPropertiesConsumer<WifiService, WifiProperties>,
          public TMessageSubscriber<WifiService, SystemEventChanged, Command> {
    WifiProperties _props;

    esp_netif_t* _netif{nullptr};
private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        auto *self = static_cast<WifiService *>(arg);
        self->eventHandler(event_base, event_id, event_data);
    }

    void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);

public:
    explicit WifiService(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "wifi";
    }

    void handle(const SystemEventChanged &msg);

    void handle(const Command &cmd);

    void apply(const WifiProperties &props);
};

