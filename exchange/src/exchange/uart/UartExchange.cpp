//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "UartExchange.h"

#ifdef CONFIG_EXCHANGE_BUS_UART

#include "UartDevice.h"
#include <hal/uart_types.h>

void UartExchange::setup() {
    _device = new UartDevice(static_cast<uart_port_t>(CONFIG_EXCHANGE_BUS_UART_NUM), CONFIG_EXCHANGE_BUS_UART_BAUD_RATE);
    AbstractExchange::setup();
}

#endif