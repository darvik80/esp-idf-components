//
// Created by Ivan Kishchenko on 19/9/24.
//

#include "Exchange.h"

uint16_t ExchangeDevice::computeChecksum(void *buf, uint16_t len) {
    auto *ptr = static_cast<uint8_t *>(buf);
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
        _rx_queue[idx] = xQueueCreate(CONFIG_EXCHANGE_BUS_RX_QUEUE_SIZE, sizeof(exchange_message_t));
        assert(_rx_queue[idx] != nullptr);

        _tx_queue[idx] = xQueueCreate(CONFIG_EXCHANGE_BUS_TX_QUEUE_SIZE, sizeof(exchange_message_t));
        assert(_tx_queue[idx] != nullptr);
    }
}

QueueHandle_t ExchangeDevice::getRxQueue(ExchangeQueuePriority priority) const {
    return _rx_queue[priority];
}

QueueHandle_t ExchangeDevice::getTxQueue(ExchangeQueuePriority priority) const {
    return _tx_queue[priority];
}

bool ExchangeDevice::hasData() {
    for (auto &queue: _tx_queue) {
        if (uxQueueMessagesWaiting(queue)) {
            return true;
        }
    }

    return false;
}

esp_err_t ExchangeDevice::unpackBuffer(void *payload, exchange_message_t &msg) {
    auto *hdr = static_cast<transfer_header_t *>(payload);
    if (hdr->stx != STX_BIT || hdr->etx != ETX_BIT) {
        return ESP_ERR_INVALID_ARG;
    }

    msg.if_type = hdr->if_type;
    msg.if_num = hdr->if_num;
    msg.flags = hdr->flags;
    msg.seq_num = hdr->seq_num;
    msg.pkt_type = hdr->pkt_type;
    msg.length = hdr->payload_len + hdr->offset;
    msg.payload = payload;

    uint16_t rx_checksum = hdr->checksum;
    hdr->checksum = 0;
    hdr->checksum = computeChecksum(msg.payload, hdr->payload_len);

    if (hdr->checksum != rx_checksum) {
        esp_loge(device, "Checksum failed: %04x vs %04x", hdr->checksum, rx_checksum);
        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
}

esp_err_t ExchangeDevice::packBuffer(const exchange_message_t &origin, exchange_message_t &msg) {
    constexpr uint16_t offset = sizeof(transfer_header_t);
    uint16_t total_len = origin.length + offset;

#ifdef CONFIG_EXCHANGE_BUS_BUFFER_DMA
    if (!IS_DMA_ALIGNED(total_len)) {
        MAKE_DMA_ALIGNED(total_len);
    }
#endif

    if (total_len > CONFIG_EXCHANGE_BUS_BUFFER) {
        esp_loge(device, "Max frame length exceeded %d.. drop it", total_len);
        return ESP_FAIL;
    }

    msg.if_type = origin.if_type;
    msg.if_num = origin.if_num;
    msg.flags = origin.flags;
    msg.seq_num = origin.seq_num;
    msg.pkt_type = origin.pkt_type;
    msg.length = total_len;
#ifdef CONFIG_EXCHANGE_BUS_BUFFER_DMA
    msg.payload = heap_caps_malloc(total_len, MALLOC_CAP_DMA);
#else
    msg.payload = malloc(total_len);
#endif
    assert(msg.payload);
    memset (msg.payload, 0, total_len);

    auto *hdr = static_cast<transfer_header_t *>(msg.payload);
    hdr->stx = STX_BIT;
    hdr->if_type = origin.if_type;
    hdr->if_num = origin.if_num;
    hdr->flags = origin.flags;
    hdr->seq_num = origin.seq_num;
    hdr->pkt_type = origin.pkt_type;
    hdr->checksum = 0;
    hdr->offset = offset;
    hdr->payload_len = origin.length;
    hdr->etx = ETX_BIT;
    /* copy the data from caller */
    if (origin.length) {
        memcpy(msg.payload + offset, origin.payload, origin.length);
    }
    hdr->checksum = computeChecksum(msg.payload, hdr->payload_len);

    return ESP_OK;
}

esp_err_t ExchangeDevice::postRxBuffer(exchange_message_t &rxBuf) const {
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

esp_err_t ExchangeDevice::getNextTxBuffer(exchange_message_t &txBuf) {
    BaseType_t ret = pdFAIL;

    for (auto &queue: _tx_queue) {
        if (ret = xQueueReceive(queue, &txBuf, 0); ret == pdTRUE) {
            break;
        }
    }

    return ret == pdTRUE && txBuf.payload ? ESP_OK : ESP_FAIL;
}

esp_err_t ExchangeDevice::writeData(const exchange_message_t &buffer, TickType_t tick) {
    exchange_message_t tx{};
    esp_err_t ret = packBuffer(buffer, tx);
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

esp_err_t ExchangeDevice::readData(exchange_message_t &buffer, TickType_t tick) {
    while (true) {
        for (auto &queue: _rx_queue) {
            if (xQueueReceive(queue, &buffer, tick)) {
                return ESP_OK;
            }
        }
        vTaskDelay(tick);
    }
}
