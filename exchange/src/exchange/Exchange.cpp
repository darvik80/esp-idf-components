//
// Created by Ivan Kishchenko on 19/9/24.
//

#include "Exchange.h"

uint16_t ExchangeDevice::computeChecksum(void *buf, uint16_t len) {
    auto* ptr = static_cast<uint8_t*>(buf);
    uint16_t checksum = 0;
    uint16_t i = 0;

    while (i < len) {
        checksum += ptr[i];
        i++;
    }

    return checksum;
}

ExchangeDevice::ExchangeDevice() {
    for (size_t idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        _rx_queue[idx] = xQueueCreate(CONFIG_EXCHANGE_BUS_RX_QUEUE_SIZE, sizeof(ExchangeMessage));
        assert(_rx_queue[idx] != nullptr);

        _tx_queue[idx] = xQueueCreate(CONFIG_EXCHANGE_BUS_TX_QUEUE_SIZE, sizeof(ExchangeMessage));
        assert(_tx_queue[idx] != nullptr);
    }
}

QueueHandle_t ExchangeDevice::getRxQueue(ExchangeQueuePriority priority) const {
    return _rx_queue[priority];
}

QueueHandle_t ExchangeDevice::getTxQueue(ExchangeQueuePriority priority) const {
    return _tx_queue[priority];
}

esp_err_t ExchangeDevice::unpackBuffer(void *payload, ExchangeMessage &msg) {
    memcpy(&msg, payload, sizeof(ExchangeHeader));
    msg.payload = payload;

    auto *header = static_cast<ExchangeHeader *>(msg.payload);
    uint16_t rxChecksum = msg.checksum;
    header->checksum = 0;
    header->checksum = computeChecksum(msg.payload, msg.payload_len);

    if ( header->checksum != rxChecksum) {
        esp_loge(device, "Checksum failed: %04x vs %04x",  header->checksum, rxChecksum);
        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
}

esp_err_t ExchangeDevice::packBuffer(const ExchangeMessage &origin, ExchangeMessage &msg, bool dma) {
    constexpr uint16_t offset = sizeof(ExchangeHeader);
    uint16_t total_len = origin.payload_len + offset;

    if (dma) {
        if (!IS_DMA_ALIGNED(total_len)) {
            MAKE_DMA_ALIGNED(total_len);
        }
    }

    if (total_len > CONFIG_EXCHANGE_BUS_BUFFER) {
        esp_loge(device, "Max frame length exceeded %d.. drop it", total_len);
        return ESP_FAIL;
    }

    memcpy(&msg, &origin, offset);

    msg.payload = dma ? heap_caps_malloc(total_len, MALLOC_CAP_DMA) : malloc(total_len);
    assert(msg.payload);
    memcpy(msg.payload, &msg, offset);
    msg.payload_len = total_len;

    /* copy the data from caller */
    if (origin.payload_len) {
        memcpy(msg.payload + offset, origin.payload, origin.payload_len);
    }
    auto* hdr = static_cast<ExchangeHeader *>(msg.payload);
    hdr->stx = STX_HDR;
    hdr->offset = offset;
    hdr->payload_len = total_len;
    msg.checksum = computeChecksum(msg.payload, msg.payload_len);
    hdr->checksum = msg.checksum;

    return ESP_OK;
}

esp_err_t ExchangeDevice::postRxBuffer(ExchangeMessage &rxBuf) const {
    BaseType_t ret = pdFAIL;
    if (rxBuf.if_type == ESP_INTERNAL_IF) {
        ret = xQueueSend(_rx_queue[PRIO_Q_HIGH], &rxBuf, portMAX_DELAY);
    } else if (rxBuf.if_type == ESP_HCI_IF) {
        ret = xQueueSend(_rx_queue[PRIO_Q_MID], &rxBuf, portMAX_DELAY);
    } else {
        ret = xQueueSend(_rx_queue[PRIO_Q_LOW], &rxBuf, portMAX_DELAY);
    }

    return ret == pdTRUE ? ESP_OK : ESP_FAIL;
}

esp_err_t ExchangeDevice::getNextTxBuffer(ExchangeMessage &txBuf) {
    BaseType_t ret = pdFAIL;

    for (auto& queue : _tx_queue) {
        if (ret = xQueueReceive(queue, &txBuf, 0); ret == pdTRUE) {
            break;
        }
    }

    return ret == pdTRUE && txBuf.payload ? ESP_OK : ESP_FAIL;
}

esp_err_t ExchangeDevice::writeData(const ExchangeMessage &buffer, TickType_t tick) {
    ExchangeMessage tx{};
    esp_err_t ret = packBuffer(buffer, tx, false);
    if (ret == ESP_OK) {
        if (tx.if_type == ESP_INTERNAL_IF) {
            ret = xQueueSend(_tx_queue[PRIO_Q_HIGH], &tx, tick) == pdTRUE ? ESP_OK : ESP_FAIL;
        } else if (tx.if_type == ESP_HCI_IF) {
            ret = xQueueSend(_tx_queue[PRIO_Q_MID], &tx, tick) == pdTRUE ? ESP_OK : ESP_FAIL;
        } else {
            ret = xQueueSend(_tx_queue[PRIO_Q_LOW], &tx, tick) == pdTRUE ? ESP_OK : ESP_FAIL;
        }
    }

    return ret;
}

esp_err_t ExchangeDevice::readData(ExchangeMessage &buffer, TickType_t tick) {
    while (true) {
        for (auto& queue : _rx_queue) {
            if (xQueueReceive(queue, &buffer, tick)) {
                return ESP_OK;
            }
        }
        vTaskDelay(tick);
    }
}
