//
// Created by Ivan Kishchenko on 29/8/24.
//

#pragma once

#include <sdkconfig.h>
#include <core/system/exchange/Exchange.h>
#ifdef CONFIG_EXCHANGE_BUS_SPI

#include <array>
#include <cstdint>

#include "SpiDevice.h"

class SpiMasterDevice final : public ExchangeDevice {
    struct spi_device_t *_spiDevice{nullptr};
    std::array<uint8_t, 2048> _rx{};
    EventGroupHandle_t _events{nullptr};

private:
    static void IRAM_ATTR gpio_handshake_isr_handler(void *arg);

    static void IRAM_ATTR gpio_ready_data_isr_handler(void *arg);

    void exchange();

protected:
    esp_err_t getNextTxBuffer(ExchangeMessage &txBuf) override;

public:
    explicit SpiMasterDevice();

    esp_err_t writeData(const ExchangeMessage &buffer, TickType_t tick) override;

    ~SpiMasterDevice() override;
};


#endif //SPIMASTERDEVICE_H
