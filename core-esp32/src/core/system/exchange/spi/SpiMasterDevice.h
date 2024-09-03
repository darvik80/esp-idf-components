//
// Created by Ivan Kishchenko on 29/8/24.
//

#ifndef SPIMASTERDEVICE_H
#define SPIMASTERDEVICE_H

#include <array>
#include <cstdint>
#include <hal/spi_types.h>

#include "SpiDevice.h"

class SpiMasterDevice final : public SpiDevice {
    spi_host_device_t _spi;
    struct spi_device_t *_spiDevice{nullptr};
    std::array<uint8_t, 2048> _rx{};
protected:
    static void IRAM_ATTR gpio_handshake_isr_handler(void *arg);
    static void IRAM_ATTR gpio_ready_data_isr_handler(void *arg);
    void run() override;

    esp_err_t postRxBuffer(SpiMessage *rx_buf_handle) const;

    void *getNextTxBuffer() const;

    void queueNextTransaction() const;

public:
    explicit SpiMasterDevice(spi_host_device_t device);

    esp_err_t setup() override;

    void destroy() override;

    esp_err_t writeData(const SpiMessage *buffer) override;

    esp_err_t readData(SpiMessage *buffer) override;

    ~SpiMasterDevice() override;
};


#endif //SPIMASTERDEVICE_H
