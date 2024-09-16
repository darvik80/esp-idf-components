//
// Created by Ivan Kishchenko on 11/9/24.
//

#pragma once

#include <freertos/FreeRTOS.h>
#include <esp_err.h>
#include <core/system/exchange/Exchange.h>
#include <driver/i2c_types.h>
#include <soc/gpio_num.h>

#define I2C_RX_BUF_SIZE              32
#define I2C_PORT 0
#define I2C_SDA_IO GPIO_NUM_8
#define I2C_SCL_IO GPIO_NUM_9

#define I2C_DEVICE_ADDRESS 0xab

#define CONFIG_I2C_RX_QUEUE_SIZE	10
#define CONFIG_I2C_TX_QUEUE_SIZE	10

struct CmdMsgLen {
    uint8_t cmd;
    uint16_t length;
} __packed;

class I2cDevice {
protected:

    virtual void exchange() = 0;
public:
    static void unpackBuffer(void* payload, ExchangeMessage &msg);

    static esp_err_t packBuffer(const ExchangeMessage &origin, ExchangeMessage &msg);

    virtual esp_err_t setup(i2c_port_num_t i2c_port, gpio_num_t sda_io_num, gpio_num_t scl_io_num) = 0;

    virtual void destroy() = 0;

    virtual esp_err_t writeData(const ExchangeMessage &buffer, TickType_t tick = portMAX_DELAY) = 0;

    virtual esp_err_t readData(ExchangeMessage &buffer, TickType_t tick = 50) = 0;

    virtual ~I2cDevice() = default;
};
