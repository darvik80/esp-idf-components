//
// Created by Ivan Kishchenko on 09/02/2024.
//

#pragma once


#include "core/system/System.h"
#include "ExtraLinuxConfig.h"

struct HidGamepadProperties : TProperties<Props_ExtraLinux_HidGamepad, Sys_ExtraLinux> {
    uint16_t vendorId;
    uint16_t productId;
};

[[maybe_unused]] void fromJson(cJSON *json, HidGamepadProperties &props);