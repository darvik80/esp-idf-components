//
// Created by Ivan Kishchenko on 29/8/24.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_EXCHANGE_BUS_SPI

#include <core/Registry.h>
#include <core/system/SystemProperties.h>
#include <core/system/SystemService.h>
#include <exchange/Exchange.h>

enum spi_mode {
    SPI_MODE_MASTER,
    SPI_MODE_SLAVE,
};

struct SpiExchangeProperties : TProperties<Props_Sys_Spi, Sys_Core> {
    spi_mode mode{SPI_MODE_MASTER};
};

[[maybe_unused]] void fromJson(cJSON *json, SpiExchangeProperties &props);

class SpiExchange : public AbstractExchange<Service_Sys_SPIExchange>,
                    public TPropertiesConsumer<SpiExchange, SpiExchangeProperties> {
public:
    explicit SpiExchange(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "spi-exchange";
    }

    void apply(const SpiExchangeProperties &props);
};

#endif //SPIEXCHANGE_H
