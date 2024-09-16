//
// Created by Ivan Kishchenko on 11/9/24.
//

#include "I2cDevice.h"

#include <cstring>
#include <core/Logger.h>

void I2cDevice::unpackBuffer(void *payload, ExchangeMessage &msg) {
    const auto *header = static_cast<struct ExchangeHeader *>(payload);
    memcpy(&msg, header, sizeof(ExchangeHeader));
    msg.payload = payload;
}

esp_err_t I2cDevice::packBuffer(const ExchangeMessage &origin, ExchangeMessage &msg) {
    uint16_t offset = sizeof(ExchangeHeader);
    uint16_t total_len = origin.length + offset;

    if (total_len > I2C_RX_BUF_SIZE) {
        esp_loge(spi_master, "Max frame length exceeded %d.. drop it", total_len);
        return ESP_FAIL;
    }

    total_len = I2C_RX_BUF_SIZE;

    //memset(&tx_buf_handle, 0, sizeof(tx_buf_handle));
    memcpy(&msg, &origin, sizeof(ExchangeHeader));
    msg.offset = offset;
    msg.length = origin.length;
    msg.payload_len = total_len;
    msg.payload = malloc(total_len);
    assert(msg.payload);
    memset(msg.payload, 0, total_len);

    /* Initialize header */
    auto *header = static_cast<struct ExchangeHeader *>(msg.payload);
    memcpy(header, &msg, sizeof(ExchangeHeader));

    /* copy the data from caller */
    if (origin.length) {
        memcpy(msg.payload + offset, origin.payload, origin.length);
    }

    return ESP_OK;
}
