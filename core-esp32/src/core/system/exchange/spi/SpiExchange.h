//
// Created by Ivan Kishchenko on 29/8/24.
//

#ifndef SPIEXCHANGE_H
#define SPIEXCHANGE_H

#include <core/Registry.h>
#include <core/system/SystemProperties.h>
#include <core/system/SystemService.h>
#include <core/system/exchange/Exchange.h>
#include <hal/spi_types.h>

#include "SpiDevice.h"

enum spi_mode {
    SPI_MODE_MASTER,
    SPI_MODE_SLAVE,
};

struct SpiExchangeProperties : TProperties<Props_Sys_Spi, Sys_Core> {
    spi_host_device_t device{SPI2_HOST};
    spi_mode mode{SPI_MODE_MASTER};
};

[[maybe_unused]] void fromJson(cJSON *json, SpiExchangeProperties &props);


class SpiExchange : public Exchange, public TService<SpiExchange, Service_Sys_SPIExchange, Sys_Core>,
                    public TPropertiesConsumer<SpiExchange, SpiExchangeProperties> {
    SpiDevice *_spi{nullptr};

private:
    void process(const ExchangeMessage& buffer);
public:
    explicit SpiExchange(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "spi-exchange";
    }

    void setup() override;

    void apply(const SpiExchangeProperties &props);

    void send(const ExchangeMessage &msg) override;

    void onMessage(const ExchangeMessage &msg) override;
};

#endif //SPIEXCHANGE_H
