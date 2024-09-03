//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "SpiMasterDevice.h"

#include <esp_timer.h>
#include <core/Task.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

void IRAM_ATTR SpiMasterDevice::gpio_handshake_isr_handler(void *arg) {
    //Sometimes due to interference or ringing or something, we get two irqs after eachother. This is solved by
    //looking at the time between interrupts and refusing any interrupt too close to another one.
    static uint32_t lasthandshaketime_us;
    uint32_t currtime_us = esp_timer_get_time();
    uint32_t diff = currtime_us - lasthandshaketime_us;
    if (diff < 1000) {
        return; //ignore everything <1ms after an earlier irq
    }
    lasthandshaketime_us = currtime_us;

    //Give the semaphore.
    auto *self = static_cast<SpiMasterDevice *>(arg);
    BaseType_t mustYield = false;
    vTaskNotifyGiveFromISR(self->getTaskHandle(), &mustYield);
    if (mustYield) {
        portYIELD_FROM_ISR(mustYield);
    }
}

void IRAM_ATTR SpiMasterDevice::gpio_ready_data_isr_handler(void *arg) {
    //Sometimes due to interference or ringing or something, we get two irqs after eachother. This is solved by
    //looking at the time between interrupts and refusing any interrupt too close to another one.
    static uint32_t lasthandshaketime_us;
    uint32_t currtime_us = esp_timer_get_time();
    uint32_t diff = currtime_us - lasthandshaketime_us;
    if (diff < 1000) {
        return; //ignore everything <1ms after an earlier irq
    }
    lasthandshaketime_us = currtime_us;

    //Give the semaphore.
    auto *self = static_cast<SpiMasterDevice *>(arg);
    BaseType_t mustYield = false;
    vTaskNotifyGiveFromISR(self->getTaskHandle(), &mustYield);
    if (mustYield) {
        portYIELD_FROM_ISR(mustYield);
    }
}

esp_err_t SpiMasterDevice::setup() {
    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT); // Setting the CS' pin to work in OUTPUT mode

    spi_bus_config_t buscfg = {
        // Provide details to the SPI_bus_sturcture of pins and maximum data size
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .mode = 0, // SPI mode 0: CPOL:-0 and CPHA:-0
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // Clock out at 12 MHz
        .spics_io_num = PIN_NUM_CS, // This field is used to specify the GPIO pin that is to be used as CS'
        .queue_size = 7, // We want to be able to queue 7 transactions at a time
    };

    //GPIO config for the handshake line.
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(PIN_NUM_MASTER_HANDSHAKE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    /* Configuration for data_ready line */
    gpio_config_t io_conf_ready = {
        .pin_bit_mask = BIT64(PIN_NUM_MASTER_DATA_READY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    gpio_config(&io_conf);
    gpio_config(&io_conf_ready);

    gpio_install_isr_service(0);
    gpio_set_intr_type(PIN_NUM_MASTER_HANDSHAKE, GPIO_INTR_POSEDGE);
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_MASTER_HANDSHAKE, gpio_handshake_isr_handler, this));

    gpio_set_intr_type(PIN_NUM_MASTER_DATA_READY, GPIO_INTR_POSEDGE);
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_MASTER_DATA_READY, gpio_ready_data_isr_handler, this));

    ESP_ERROR_CHECK(spi_bus_initialize(_spi, &buscfg, SPI_DMA_CH_AUTO)); // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(_spi, &devcfg, &_spiDevice)); // Attach the Slave device to the SPI bus

    //Assume the slave is ready for the first transmission: if the slave started up before us, we will not detect
    //positive edge on the handshake line.
    return SpiDevice::setup();
}

void SpiMasterDevice::run() {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        spi_transaction_t spi_trans {
            .length = RX_BUF_SIZE * SPI_BITS_PER_WORD,
            .tx_buffer = getNextTxBuffer(),
            .rx_buffer = heap_caps_malloc(RX_BUF_SIZE, MALLOC_CAP_DMA),
        };
        memset(spi_trans.rx_buffer, 0, RX_BUF_SIZE);

        if (auto err = spi_device_transmit(_spiDevice, &spi_trans); err != ESP_OK) {
            esp_loge(spi_master, "spi transmit error, err: 0x%x (%s)", err, esp_err_to_name(err));
            free(spi_trans.rx_buffer);
            free((void *) spi_trans.tx_buffer);
        } else {
            /* Free any tx buffer, data is not relevant anymore */
            if (spi_trans.tx_buffer) {
                free(const_cast<void *>(spi_trans.tx_buffer));
            }

            /* Process received data */
            if (spi_trans.rx_buffer) {
                SpiMessage rx_buf_handle{};
                unpackBuffer(spi_trans.rx_buffer, rx_buf_handle);
                if (err = postRxBuffer(&rx_buf_handle); err != ESP_OK) {
                    free(spi_trans.rx_buffer);
                }
            }
        }
    }
}

void SpiMasterDevice::destroy() {
    SpiDevice::destroy();
}

SpiMasterDevice::SpiMasterDevice(spi_host_device_t device) : _spi(device) {
}

void *SpiMasterDevice::getNextTxBuffer() const {
    SpiMessage buf_handle{0};
    BaseType_t ret = pdFAIL;

    /* Get or create new tx_buffer
     *	1. Check if SPI TX queue has pending buffers. Return if valid buffer is obtained.
     *	2. Create a new empty tx buffer and return */
    for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        if (ret = xQueueReceive(getTxQueue(idx), &buf_handle, 0); ret == pdTRUE) {
            break;
        }
    }

    if (ret == pdTRUE && buf_handle.payload) {
        return buf_handle.payload;
    }

    // /* Create empty dummy buffer */
    void *sendbuf = heap_caps_malloc(RX_BUF_SIZE, MALLOC_CAP_DMA);
    if (!sendbuf) {
        esp_logi(spi_master, "Failed to allocate memory for dummy transaction");
        return nullptr;
    }
    memset(sendbuf, 0, RX_BUF_SIZE);
    return sendbuf;
}

esp_err_t SpiMasterDevice::postRxBuffer(SpiMessage *rx_buf_handle) const {
    BaseType_t ret = ESP_OK;
    if (rx_buf_handle->if_type == ESP_INTERNAL_IF) {
        ret = xQueueSend(getRxQueue(PRIO_Q_HIGH), rx_buf_handle, portMAX_DELAY);
    } else if (rx_buf_handle->if_type == ESP_HCI_IF) {
        ret = xQueueSend(getRxQueue(PRIO_Q_MID), rx_buf_handle, portMAX_DELAY);
    } else {
        ret = xQueueSend(getRxQueue(PRIO_Q_LOW), rx_buf_handle, portMAX_DELAY);
    }

    return ret == pdTRUE ? ESP_OK : ESP_FAIL;
}

void SpiMasterDevice::queueNextTransaction() const {
    void *tx_buffer = getNextTxBuffer();
    if (!tx_buffer) {
        /* Queue next transaction failed */
        esp_loge(spi_master, "Failed to queue new transaction");
        return;
    }

    auto *spi_trans = static_cast<spi_transaction_t *>(malloc(sizeof(spi_transaction_t)));
    assert(spi_trans);

    memset(spi_trans, 0, sizeof(spi_transaction_t));

    /* Attach Rx Buffer */
    spi_trans->rx_buffer = heap_caps_malloc(RX_BUF_SIZE, MALLOC_CAP_DMA);
    assert(spi_trans->rx_buffer);
    memset(spi_trans->rx_buffer, 0, RX_BUF_SIZE);

    /* Attach Tx Buffer */
    spi_trans->tx_buffer = tx_buffer;

    /* Transaction len */
    spi_trans->length = RX_BUF_SIZE * SPI_BITS_PER_WORD;

    esp_err_t ret = spi_device_queue_trans(_spiDevice, spi_trans, portMAX_DELAY);
    if (ret != ESP_OK) {
        esp_loge(spi_master, "Failed to queue next SPI transfer\n");
        free(spi_trans->rx_buffer);
        spi_trans->rx_buffer = nullptr;
        free(const_cast<void *>(spi_trans->tx_buffer));
        spi_trans->tx_buffer = nullptr;
        free(spi_trans);
    }
}

esp_err_t SpiMasterDevice::writeData(const SpiMessage *buffer) {
    gpio_set_level(PIN_NUM_CS, 0); // Lower the CS' line to select the slave
    SpiMessage tx_buf_handle = {0};

    uint16_t offset = sizeof(SpiHeader);
    uint16_t total_len = buffer->length + offset;

    /* make the adresses dma aligned */
    if (!IS_SPI_DMA_ALIGNED(total_len)) {
        MAKE_SPI_DMA_ALIGNED(total_len);
    }

    if (total_len > RX_BUF_SIZE) {
        esp_loge(spi_master, "Max frame length exceeded %d.. drop it", total_len);
        return ESP_FAIL;
    }

    memset(&tx_buf_handle, 0, sizeof(tx_buf_handle));
    tx_buf_handle.if_type = buffer->if_type;
    tx_buf_handle.if_num = buffer->if_num;
    tx_buf_handle.length = buffer->length;
    tx_buf_handle.offset = offset;
    tx_buf_handle.payload_len = total_len;
    tx_buf_handle.pkt_type = buffer->pkt_type;

    tx_buf_handle.payload = heap_caps_malloc(total_len, MALLOC_CAP_DMA);
    assert(tx_buf_handle.payload);
    memset(tx_buf_handle.payload, 0, total_len);

    /* Initialize header */
    auto *header = static_cast<struct SpiHeader *>(tx_buf_handle.payload);
    header->if_type = buffer->if_type;
    header->if_num = buffer->if_num;
    header->length = buffer->length;
    header->payload_len = total_len;
    header->offset = offset;
    header->flags = buffer->flags;
    header->pkt_type = buffer->pkt_type;

    /* copy the data from caller */
    if (buffer->payload_len) {
        memcpy(tx_buf_handle.payload + offset, buffer->payload, buffer->payload_len);
    }

    esp_err_t ret = ESP_OK;
    if (tx_buf_handle.if_type == ESP_INTERNAL_IF) {
        ret = xQueueSend(getTxQueue(PRIO_Q_HIGH), &tx_buf_handle, portMAX_DELAY);
    } else if (tx_buf_handle.if_type == ESP_HCI_IF) {
        ret = xQueueSend(getTxQueue(PRIO_Q_MID), &tx_buf_handle, portMAX_DELAY);
    } else {
        ret = xQueueSend(getTxQueue(PRIO_Q_LOW), &tx_buf_handle, portMAX_DELAY);
    }

    if (ret != pdTRUE) {
        return ESP_FAIL;
    }

    // After CS' is high, the slave sill get unselected
    gpio_set_level(PIN_NUM_CS, 1);
    xTaskNotifyGive(getTaskHandle());

    return ESP_OK;
}

esp_err_t SpiMasterDevice::readData(SpiMessage *buffer) {
    while (true) {
        for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
            if (xQueueReceive(getRxQueue(idx), buffer, 0)) {
                return ESP_OK;
            }
        }
        vTaskDelay(1);
    }
}

SpiMasterDevice::~SpiMasterDevice() {
    spi_bus_remove_device(_spiDevice);
    spi_bus_free(_spi);
}
