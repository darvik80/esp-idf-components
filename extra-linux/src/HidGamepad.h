//
// Created by Ivan Kishchenko on 25/01/2024.
//

#pragma once

#include "core/Core.h"
#include "UserService.h"
#include <hidapi/hidapi.h>
#include "HidGamepadProperties.h"
#include <core/v2/FreeRTOSCpp.h>

class HidGamepad : public TService<HidGamepad, Service_User_LinuxHidGamepad, Sys_User>
        , public TPropertiesConsumer<HidGamepad, HidGamepadProperties> {
    HidGamepadProperties _props;

    //freertos::Task _task;
private:
    void doProcess();
public:
    explicit HidGamepad(Registry &registry);

    void apply(const HidGamepadProperties &props);

    void setup() override;

    [[noreturn]] [[noreturn]] void handle();
    ~HidGamepad() override;
};
