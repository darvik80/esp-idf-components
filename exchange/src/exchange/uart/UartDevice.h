//
// Created by Ivan Kishchenko on 19/9/24.
//

#pragma once

#include <sdkconfig.h>

#ifdef CONFIG_EXCHANGE_BUS_UART

#include <exchange/Exchange.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <hal/uart_types.h>

class UartDevice : public ExchangeDevice {
    QueueHandle_t _rxQueue{};
    uart_port_t _port{UART_NUM_MAX};

private:
    void rxTask();

    void txTask();

public:
    UartDevice(uart_port_t port, int baudRate);
};

#endif
