//
// Created by Ivan Kishchenko on 11/9/24.
//

#pragma once

#include <array>

#include "I2cDevice.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <driver/i2c_slave.h>

class I2cSlaveDevice : public I2cDevice {
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _rx_queue{};
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _tx_queue{};

    QueueHandle_t _dataQueue;
    i2c_slave_dev_handle_t _device{};
private:
    esp_err_t getNextTxBuffer(ExchangeMessage& buf) const;
    esp_err_t postRxBuffer(ExchangeMessage &buf) const;

    static IRAM_ATTR bool recvDoneCallback(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data, void *arg);
protected:
    void exchange() override;
public:
    esp_err_t setup(i2c_port_num_t i2c_port, gpio_num_t sda_io_num, gpio_num_t scl_io_num) override;

    esp_err_t writeData(const ExchangeMessage &buffer, TickType_t tick) override;

    esp_err_t readData(ExchangeMessage &buffer, TickType_t tick) override;

    void destroy() override;

};

