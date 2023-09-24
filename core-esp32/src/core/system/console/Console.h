//
// Created by Ivan Kishchenko on 21/09/2023.
//

#pragma once

#include "core/Core.h"
#include "core/Registry.h"
#include <esp_event.h>
#include <freertos/event_groups.h>
#include <esp_netif.h>
#include "core/Logger.h"
#include "core/system/SystemEvent.h"


class UartConsoleService : public TService<UartConsoleService, Service_Sys_Console, Sys_Core> {
public:
    explicit UartConsoleService(Registry &registry);

    void setup() override;
};
