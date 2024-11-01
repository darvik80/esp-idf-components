//
// Created by Ivan Kishchenko on 29/8/24.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_EXCHANGE_BUS_SPI
#include <exchange/Exchange.h>

#include <array>
#include <driver/spi_slave.h>
#include "SpiDevice.h"

class SpiSlaveDevice : public ExchangeDevice {
    std::array<uint8_t, 2048> _rx{};

private:

    static void postSetupCb(spi_slave_transaction_t *);

    //Called after transaction is sent/received. We use this to set the handshake line low.
    static void postTransCb(spi_slave_transaction_t *trans);

protected:
    esp_err_t getNextTxBuffer(exchange_message_t &txBuf);

    void queueNextTransaction() const;

    void exchange();

public:
    explicit SpiSlaveDevice();


    esp_err_t writeData(const exchange_message_t &buffer, TickType_t tick) override;

    ~SpiSlaveDevice() override;
};

#endif
