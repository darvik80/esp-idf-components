//
// Created by Ivan Kishchenko on 30/8/24.
//

#include "SpiDevice.h"

#include <sys/unistd.h>

void SpiDevice::unpackBuffer(void* payload, SpiMessage &msg) {
    const auto *header = static_cast<struct SpiHeader *>(payload);
    msg.if_type = header->if_type;
    msg.if_num = header->if_num,
    msg.offset = header->offset,
    msg.length = header->length,
    msg.payload_len = header->payload_len;
    msg.payload = payload;
}

SpiDevice::SpiDevice() {
    for (size_t idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        _rx_queue[idx] = xQueueCreate(CONFIG_SPI_RX_QUEUE_SIZE, sizeof(SpiMessage));
        assert(_rx_queue[idx] != nullptr);

        _tx_queue[idx] = xQueueCreate(CONFIG_SPI_TX_QUEUE_SIZE, sizeof(SpiMessage));
        assert(_tx_queue[idx] != nullptr);
    }
}

esp_err_t SpiDevice::setup() {

    assert(xTaskCreate(
        SpiDevice::run ,
        "spi-device-task" ,
        SPI_TASK_DEFAULT_STACK_SIZE,
        this ,
        SPI_TASK_DEFAULT_PRIO, &_task
    ) == pdTRUE);

    usleep(500);

    xTaskNotifyGive(getTaskHandle());
    return ESP_OK;
}

void SpiDevice::destroy() {
    vTaskDelete(_task);

}

SpiDevice::~SpiDevice() {
    for (size_t idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        vQueueDelete(_rx_queue[idx]);
        vQueueDelete(_tx_queue[idx]);
    }
}
