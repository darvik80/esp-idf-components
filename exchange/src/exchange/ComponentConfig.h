//
// Created by Ivan Kishchenko on 4/10/24.
//

#pragma once

#include <sdkconfig.h>

enum ComponentServiceId {
    Service_Sys_UartExchange,
    Service_Sys_I2CExchange,
    Service_Sys_SPIExchange,
    Service_Sys_EspNowExchange,
};


enum Component {
    Component_Exchange = CONFIG_EXCHANGE_BUS_COMPONENT_ID,
};