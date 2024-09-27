//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "SpiSlaveDevice.h"

#ifdef CONFIG_EXCHANGE_BUS_SPI

#include <cstring>
#include <endian.h>
#include <esp_timer.h>
#include <core/Task.h>
#include <driver/gpio.h>
#include <driver/spi_slave.h>
#include <soc/gpio_reg.h>

//Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
void SpiSlaveDevice::postSetupCb(spi_slave_transaction_t *trans) {
    gpio_set_level(pinHandshake, 1);
}

//Called after transaction is sent/received. We use this to set the handshake line low.
void SpiSlaveDevice::postTransCb(spi_slave_transaction_t *trans) {
    gpio_set_level(pinHandshake, 0);
}

 SpiSlaveDevice::SpiSlaveDevice() {
    //Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_EXCHANGE_BUS_SPI_MOSI_PIN,
        .miso_io_num = CONFIG_EXCHANGE_BUS_SPI_MISO_PIN,
        .sclk_io_num = CONFIG_EXCHANGE_BUS_SPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = CONFIG_EXCHANGE_BUS_SPI_CS_PIN,
        .flags = 0,
        .queue_size = 7,
        .mode = 0,
        .post_setup_cb = postSetupCb,
        .post_trans_cb = postTransCb,
    };

    /* Configuration for the handshake line */
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(pinHandshake),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };

    /* Configuration for data_ready line */
    gpio_config_t io_conf_ready = {
        .pin_bit_mask = BIT64(pinDataReady),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };

    //Configure handshake line as output
    gpio_config(&io_conf);
    gpio_config(&io_conf_ready);
    gpio_set_level(pinHandshake, 0);
    gpio_set_level(pinDataReady, 0);

    //Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode(pinMOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(pinCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(pinCS, GPIO_PULLUP_ONLY);

    //Initialize SPI slave interface
    ESP_ERROR_CHECK(spi_slave_initialize(spi, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));

    FreeRTOSTask::execute([this]() {
        exchange();
    }, "spi-device-task", SPI_TASK_DEFAULT_STACK_SIZE, SPI_TASK_DEFAULT_PRIO);

    usleep(500);
}

void SpiSlaveDevice::exchange() {
    while (true) {
        ExchangeMessage txBuf{}, rxBuf{};
        if (ESP_OK != getNextTxBuffer(txBuf)) {
            continue;
        }

        esp_logd(spi_slave, "have data for master: %d", txBuf.payload_len);
        spi_slave_transaction_t spi_trans{
            .length = CONFIG_EXCHANGE_BUS_BUFFER * SPI_BITS_PER_WORD,
            .tx_buffer = txBuf.payload,
            .rx_buffer = heap_caps_malloc(CONFIG_EXCHANGE_BUS_BUFFER, MALLOC_CAP_DMA),
        };
        memset(spi_trans.rx_buffer, 0, CONFIG_EXCHANGE_BUS_BUFFER);

        if (auto err = spi_slave_transmit(spi, &spi_trans, portMAX_DELAY); err != ESP_OK) {
            esp_loge(spi_slave, "spi transmit error, err: 0x%x (%s)", err, esp_err_to_name(err));
            free(spi_trans.rx_buffer);
            free((void *) spi_trans.tx_buffer);
            continue;
        }

        /* Free any tx buffer, data is not relevant anymore */
        if (spi_trans.tx_buffer) {
            free((void *) spi_trans.tx_buffer);
        }

        /* Process received data */
        if (spi_trans.rx_buffer) {
            /* Process received data */
            if (spi_trans.rx_buffer) {
                if (ESP_OK != unpackBuffer(spi_trans.rx_buffer, rxBuf)) {
                    free(spi_trans.rx_buffer);
                    continue;
                }
                if (rxBuf.if_type == 0x0f && rxBuf.if_num == 0x0f) {
                    free(spi_trans.rx_buffer);
                    continue;
                }

                if (ESP_OK != postRxBuffer(rxBuf)) {
                    free(spi_trans.rx_buffer);
                }
            }
        }
    }
}

esp_err_t SpiSlaveDevice::getNextTxBuffer(ExchangeMessage &txBuf) {
    if (ESP_OK == ExchangeDevice::getNextTxBuffer(txBuf)) {
        return ESP_OK;
    }

    gpio_set_level(pinDataReady, 0);
    ExchangeMessage dummy{
        ExchangeHeader{
            .if_type = 0xF,
            .if_num = 0xF,
        },
    };
    return packBuffer(dummy, txBuf, true);
}

esp_err_t SpiSlaveDevice::writeData(const ExchangeMessage &buffer, TickType_t tick) {
    auto err = ExchangeDevice::writeData(buffer, tick);
    if (ESP_OK == err) {
        gpio_set_level(pinDataReady, 1);
    }

    return err;
}

SpiSlaveDevice::~SpiSlaveDevice() {
    if (auto err = spi_slave_free(spi); err != ESP_OK) {
        esp_loge(spi_slave, "spi slave bus free failed, %s", esp_err_to_name(err));
    }
    if (auto err = spi_bus_free(spi); err != ESP_OK) {
        esp_loge(spi_slave, "spi all bus free failed, %s", esp_err_to_name(err));
    }
}

#endif
