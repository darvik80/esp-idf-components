//
// Created by Ivan Kishchenko on 12/10/2023.
//

#pragma once

#include "core/system/System.h"

struct WifiProperties : TProperties<Props_Sys_Wifi, System::Sys_Core> {
    struct AccessPoint {
        std::string ssid;
        std::string password;
    };
    std::vector<AccessPoint> ap;
};

[[maybe_unused]] void fromJson(cJSON *json, WifiProperties &props);
