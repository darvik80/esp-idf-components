//
// Created by Ivan Kishchenko on 4/10/24.
//

#include "EspNowExchange.h"

#ifdef CONFIG_EXCHANGE_BUS_ESP_NOW

#include "EspNowDevice.h"

void EspNowExchange::setup() {
    _device = new EspNowDevice();
    AbstractExchange::setup();
}

#endif