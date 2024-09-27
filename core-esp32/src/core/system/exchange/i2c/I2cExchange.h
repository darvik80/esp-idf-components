//
// Created by Ivan Kishchenko on 29/8/24.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_EXCHANGE_BUS_I2C

#include <hal/i2c_types.h>
#include <core/Registry.h>
#include <core/system/SystemProperties.h>
#include <core/system/SystemService.h>

#include "core/system/exchange/Exchange.h"
#include "core/system/exchange/i2c/I2cDevice.h"

struct I2cExchangeProperties : TProperties<Props_Sys_I2c, Sys_Core> {
    i2c_mode_t mode;
};

[[maybe_unused]] void fromJson(cJSON *json, I2cExchangeProperties &props);


class I2cExchange final : public AbstractExchange<Service_Sys_I2CExchange>,
                          public TPropertiesConsumer<I2cExchange, I2cExchangeProperties> {
public:
    explicit I2cExchange(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "i2c-exchange";
    }

    void apply(const I2cExchangeProperties &props);
};

#endif
