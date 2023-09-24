//
// Created by Ivan Kishchenko on 19/08/2023.
//


#pragma once

enum SystemPropId {
    Props_Sys_Wifi,
    Props_Sys_Mqtt,
};

enum SystemServiceId {
    Service_Sys_Wifi,
    Service_Sys_Mqtt,
    Service_Sys_Console,
    Service_Sys_Telemetry,
};

enum System {
    Sys_Core = 0x01,
    Sys_User = 0x02,
};