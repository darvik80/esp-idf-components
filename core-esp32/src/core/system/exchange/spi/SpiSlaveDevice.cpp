//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "SpiSlaveDevice.h"

#include <cstring>
#include <endian.h>
#include <esp_timer.h>
#include <core/Task.h>
#include <driver/gpio.h>
#include <driver/spi_slave.h>
#include <soc/gpio_reg.h>

//Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
void SpiSlaveDevice::postSetupCb(spi_slave_transaction_t *trans) {
    gpio_set_level(PIN_NUM_SLAVE_HANDSHAKE, 1);
}

//Called after transaction is sent/received. We use this to set the handshake line low.
void SpiSlaveDevice::postTransCb(spi_slave_transaction_t *trans) {
    gpio_set_level(PIN_NUM_SLAVE_HANDSHAKE, 0);
}

esp_err_t SpiSlaveDevice::setup() {
    //Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = PIN_NUM_CS,
        .flags = 0,
        .queue_size = 7,
        .mode = 0,
        .post_setup_cb = postSetupCb,
        .post_trans_cb = postTransCb,
    };

    /* Configuration for the handshake line */
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(PIN_NUM_SLAVE_HANDSHAKE),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };

    /* Configuration for data_ready line */
    gpio_config_t io_conf_ready = {
        .pin_bit_mask = BIT64(PIN_NUM_SLAVE_DATA_READY),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };

    //Configure handshake line as output
    gpio_config(&io_conf);
    gpio_config(&io_conf_ready);
    gpio_set_level(PIN_NUM_SLAVE_HANDSHAKE, 0);
    gpio_set_level(PIN_NUM_SLAVE_DATA_READY, 0);

    //Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode(PIN_NUM_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_CLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_CS, GPIO_PULLUP_ONLY);

    //Initialize SPI slave interface
    ESP_ERROR_CHECK(spi_slave_initialize(_spi, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));

    return SpiDevice::setup();
}

void SpiSlaveDevice::run() {
    uint32_t lastTime = esp_timer_get_time();
    int bytes{0};
    while (true) {
        spi_slave_transaction_t spi_trans{
            .length = RX_BUF_SIZE * SPI_BITS_PER_WORD,
            .tx_buffer = getNextTxBuffer(),
            .rx_buffer = heap_caps_malloc(RX_BUF_SIZE, MALLOC_CAP_DMA),
        };
        memset(spi_trans.rx_buffer, 0, RX_BUF_SIZE);

        if (auto err = spi_slave_transmit(_spi, &spi_trans, portMAX_DELAY); err != ESP_OK) {
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
            auto* header = (SpiHeader*)spi_trans.rx_buffer;
            if (header->if_type == 0x0f && header->if_num == 0x0f) {
                esp_logd(spi_master, "drop dummy message");
                free(spi_trans.rx_buffer);
            } else {
                bytes += RX_BUF_SIZE;
                int64_t now = esp_timer_get_time();
                if ((now-lastTime) > 1000000) {
                    esp_logi(spy, "%d b/s", bytes*8);
                    lastTime=now;
                    bytes=0;
                }
                SpiMessage rx_buf_handle{};
                unpackBuffer(spi_trans.rx_buffer, rx_buf_handle);
                if (ESP_OK != postRxBuffer(&rx_buf_handle)) {
                    free(spi_trans.rx_buffer);
                }
            }
        }
    }
}

void *SpiSlaveDevice::getNextTxBuffer() const {
    SpiMessage buf_handle{0};
    BaseType_t ret = pdFALSE;

    /* Get or create new tx_buffer
     *	1. Check if SPI TX queue has pending buffers. Return if valid buffer is obtained.
     *	2. Create a new empty tx buffer and return */

    /* Get buffer from SPI Tx queue */
    for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        if (ret = xQueueReceive(getTxQueue(idx), &buf_handle, 0); ret == pdTRUE) {
            break;
        }
    }

    if (ret == pdTRUE && buf_handle.payload) {
        // // { Maybe better do not reset data is ready
        // for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        //     if (uxQueueMessagesWaiting(getTxQueue(idx))) {
        //         return buf_handle.payload;
        //     }
        // }
        //
        // gpio_set_level(PIN_NUM_SLAVE_DATA_READY, 0);
        // // } Maybe better do not reset data is ready
        return buf_handle.payload;
    }

    /* No real data pending, clear ready line and indicate host an idle state */
    gpio_set_level(PIN_NUM_SLAVE_DATA_READY, 0);

    /* Create empty dummy buffer */
    void *sendbuf = heap_caps_malloc(RX_BUF_SIZE, MALLOC_CAP_DMA);
    if (!sendbuf) {
        esp_logi(spi_slave, "Failed to allocate memory for dummy transaction");
        return nullptr;
    }

    memset(sendbuf, 0, RX_BUF_SIZE);
    /* Initialize header */
    auto *header = static_cast<SpiHeader *>(sendbuf);
    /* Populate header to indicate it as a dummy buffer */
    header->if_type = 0xF;
    header->if_num = 0xF;

    return sendbuf;
}

esp_err_t SpiSlaveDevice::postRxBuffer(SpiMessage *rx_buf_handle) const {
    BaseType_t ret = pdFALSE;
    if (rx_buf_handle->if_type == ESP_INTERNAL_IF) {
        ret = xQueueSend(getRxQueue(PRIO_Q_HIGH), rx_buf_handle, portMAX_DELAY);
    } else if (rx_buf_handle->if_type == ESP_HCI_IF) {
        ret = xQueueSend(getRxQueue(PRIO_Q_MID), rx_buf_handle, portMAX_DELAY);
    } else {
        ret = xQueueSend(getRxQueue(PRIO_Q_LOW), rx_buf_handle, portMAX_DELAY);
    }

    return ret == pdTRUE ? ESP_OK : ESP_FAIL;
}

SpiSlaveDevice::SpiSlaveDevice(spi_host_device_t device)
    : _spi(device) {
}

esp_err_t SpiSlaveDevice::writeData(const SpiMessage *buffer) {
    SpiMessage tx_buf_handle = {0};

    uint16_t offset = sizeof(SpiHeader);
    uint16_t total_len = buffer->length + offset;

    /* make the adresses dma aligned */
    if (!IS_SPI_DMA_ALIGNED(total_len)) {
        MAKE_SPI_DMA_ALIGNED(total_len);
    }

    if (total_len > RX_BUF_SIZE) {
        esp_loge(spi_slave, "Max frame length exceeded %d.. drop it", total_len);
        return ESP_FAIL;
    }

    memset(&tx_buf_handle, 0, sizeof(tx_buf_handle));
    tx_buf_handle.if_type = buffer->if_type;
    tx_buf_handle.if_num = buffer->if_num;
    tx_buf_handle.offset = offset;
    tx_buf_handle.length = buffer->length;
    tx_buf_handle.payload_len = total_len;
    tx_buf_handle.pkt_type = buffer->pkt_type;

    tx_buf_handle.payload = heap_caps_malloc(total_len, MALLOC_CAP_DMA);
    assert(tx_buf_handle.payload);
    memset(tx_buf_handle.payload, 0, total_len);

    /* Initialize header */
    auto *header = static_cast<SpiHeader *>(tx_buf_handle.payload);
    header->if_type = buffer->if_type;
    header->if_num = buffer->if_num;
    header->offset = offset;
    header->length = buffer->length;
    header->payload_len = total_len;
    header->flags = buffer->flags;
    header->pkt_type = buffer->pkt_type;

    /* copy the data from caller */
    if (buffer->payload_len) {
        memcpy(tx_buf_handle.payload + offset, buffer->payload, buffer->payload_len);
    }

    BaseType_t ret{};
    if (tx_buf_handle.if_type == ESP_INTERNAL_IF) {
        ret = xQueueSend(getTxQueue(PRIO_Q_HIGH), &tx_buf_handle, portMAX_DELAY);
    } else if (tx_buf_handle.if_type == ESP_HCI_IF) {
        ret = xQueueSend(getTxQueue(PRIO_Q_MID), &tx_buf_handle, portMAX_DELAY);
    } else {
        ret = xQueueSend(getTxQueue(PRIO_Q_LOW), &tx_buf_handle, portMAX_DELAY);
    }

    if (ret == pdTRUE) {
        gpio_set_level(PIN_NUM_SLAVE_DATA_READY, 1);
    }


    return ret == pdTRUE ? ESP_OK : ESP_FAIL;
}

esp_err_t SpiSlaveDevice::readData(SpiMessage *buffer) {
    while (true) {
        for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
            if (xQueueReceive(getRxQueue(idx), buffer, 0)) {
                return ESP_OK;
            }
        }
        vTaskDelay(1);
    }
}

void SpiSlaveDevice::destroy() {
    if (auto err = spi_slave_free(_spi); err != ESP_OK) {
        esp_loge(spi_slave, "spi slave bus free failed, %s", esp_err_to_name(err));
    }
    if (auto err = spi_bus_free(_spi); err != ESP_OK) {
        esp_loge(spi_slave, "spi all bus free failed, %s", esp_err_to_name(err));
    }

    SpiDevice::destroy();
}
