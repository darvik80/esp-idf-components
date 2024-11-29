//
// Created by Ivan Kishchenko on 11/9/24.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_EXCHANGE_BUS_I2C
#include <exchange/Exchange.h>

#include <soc/gpio_num.h>

constexpr gpio_num_t pinSDA{static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_I2C_SDA_PIN)};
constexpr gpio_num_t pinSCL{static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_I2C_SCL_PIN)};


class I2cDevice : public ExchangeDevice {
protected:
    esp_err_t packBuffer(const ExchangeMessage &origin, ExchangeMessage &msg) override {
        constexpr uint16_t offset = sizeof(ExchangeHeader);
        uint16_t total_len = origin.payload_len + offset;

        if (total_len > CONFIG_EXCHANGE_BUS_BUFFER) {
            esp_loge(device, "Max frame length exceeded %d.. drop it", total_len);
            return ESP_FAIL;
        }

        memcpy(&msg, &origin, offset);

        msg.payload = malloc(CONFIG_EXCHANGE_BUS_BUFFER+1);
        assert(msg.payload);
        memcpy(msg.payload, &msg, offset);
        msg.payload_len = total_len;

        /* copy the data from caller */
        if (origin.payload_len) {
            memcpy(msg.payload + offset, origin.payload, origin.payload_len);
        }
        auto *hdr = static_cast<ExchangeHeader *>(msg.payload);
        hdr->stx = STX_HDR;
        hdr->offset = offset;
        hdr->payload_len = total_len;
        msg.checksum = computeChecksum(msg.payload, msg.payload_len);
        hdr->checksum = msg.checksum;

        return ESP_OK;
    }

    esp_err_t getNextTxBuffer(ExchangeMessage &txBuf) {
        if (ESP_OK == ExchangeDevice::getNextTxBuffer(txBuf)) {
            return ESP_OK;
        }

        ExchangeMessage dummy{
            ExchangeHeader{
                .if_type = 0xF,
                .if_num = 0xF,
            },
        };
        return packBuffer(dummy, txBuf);

    }
};

#endif
