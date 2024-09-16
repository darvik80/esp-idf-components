//
// Created by Ivan Kishchenko on 29/8/24.
//

#pragma once

#include <core/Registry.h>
#include <core/system/SystemProperties.h>
#include <core/system/SystemService.h>

#include "core/system/exchange/Exchange.h"
#include "core/system/exchange/i2c/I2cDevice.h"

struct I2cExchangeProperties : TProperties<Props_Sys_Spi, Sys_Core> {
    i2c_mode_t mode;
};

[[maybe_unused]] void fromJson(cJSON *json, I2cExchangeProperties &props);


class I2cExchange : public TService<I2cExchange, Service_Sys_I2CExchange, Sys_Core>,
                    public TPropertiesConsumer<I2cExchange, I2cExchangeProperties> {
    I2cDevice* _device{};
private:
    void process(const ExchangeMessage& buffer);
public:
    explicit I2cExchange(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "i2c-exchange";
    }

    void setup() override;

    void apply(const I2cExchangeProperties &props);
};

