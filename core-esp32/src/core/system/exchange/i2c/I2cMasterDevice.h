//
// Created by Ivan Kishchenko on 11/9/24.
//

#pragma once

#include <sdkconfig.h>

#ifdef CONFIG_EXCHANGE_BUS_I2C

#include "I2cDevice.h"

#include <driver/i2c_master.h>

class I2cMasterDevice : public I2cDevice {
    i2c_master_bus_handle_t _bus{};
    i2c_master_dev_handle_t _device{};
protected:
    void exchange();
public:
    I2cMasterDevice();
};

#endif
