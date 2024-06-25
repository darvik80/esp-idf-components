//
// Created by Ivan Kishchenko on 12/10/2023.
//

#pragma once

enum SystemServiceId {
    Service_Sys_NvsStorage,
    Service_Sys_Wifi,
    Service_Sys_Mqtt,
    Service_Sys_Console,
    Service_Sys_Telemetry,
    Service_Sys_Ota,
    Service_Sys_UartExchange,
    Service_Sys_I2CExchange,
    Service_Sys_SPIExchange,
};

enum System {
    Sys_Core = 0x01,
    Sys_User = 0x02,
};