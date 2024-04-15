//
// Created by Ivan Kishchenko on 21/09/2023.
//

#pragma once

#include <esp_event.h>
#include <freertos/event_groups.h>
#include "core/system/System.h"

class UartConsoleService : public TService<UartConsoleService, Service_Sys_Console, Sys_Core> {
public:
    UartConsoleService() = delete;

    UartConsoleService(const UartConsoleService &) = delete;

    UartConsoleService &operator=(const UartConsoleService &) = delete;

    explicit UartConsoleService(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "console";
    }

    void setup() override;
};
