//
// Created by Ivan Kishchenko on 21/09/2023.
//

#pragma once

#include <esp_event.h>
#include <freertos/event_groups.h>
#include "core/system/System.h"

class UartConsoleService : public TService<UartConsoleService, Service_Sys_Console, Sys_Core> {
public:
    explicit UartConsoleService(Registry &registry);

    void setup() override;
};
