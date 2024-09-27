//
// Created by Ivan Kishchenko on 11/9/24.
//

#pragma once

#include <sdkconfig.h>

#ifdef CONFIG_EXCHANGE_BUS_I2C

#include "I2cDevice.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <driver/i2c_slave.h>

class I2cSlaveDevice : public I2cDevice {

    QueueHandle_t _dataQueue;
    i2c_slave_dev_handle_t _device{};
private:
    static IRAM_ATTR bool recvDoneCallback(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data, void *arg);
protected:
    void exchange();
public:
    I2cSlaveDevice();
};

#endif