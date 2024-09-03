//
// Created by Ivan Kishchenko on 29/8/24.
//

#ifndef SPISLAVEDEVICE_H
#define SPISLAVEDEVICE_H

#include <array>
#include <driver/spi_slave.h>
#include "SpiDevice.h"

class SpiSlaveDevice : public SpiDevice {
    spi_host_device_t _spi;
    std::array<uint8_t, 2048> _rx{};

private:

    static void postSetupCb(spi_slave_transaction_t *);

    //Called after transaction is sent/received. We use this to set the handshake line low.
    static void postTransCb(spi_slave_transaction_t *trans);

protected:
    esp_err_t postRxBuffer(SpiMessage *buf_handle) const;

    void *getNextTxBuffer() const;

    void queueNextTransaction() const;

    void run() override;

public:
    explicit SpiSlaveDevice(spi_host_device_t device);

    esp_err_t setup() override;

    void destroy() override;

    esp_err_t writeData(const SpiMessage *buffer) override;

    esp_err_t readData(SpiMessage *buffer) override;
};

#endif //SPISLAVEDEVICE_H
