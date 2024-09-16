//
// Created by Ivan Kishchenko on 11/9/24.
//

#include "I2cSlaveDevice.h"

#include <core/Logger.h>
#include <core/Task.h>

bool I2cSlaveDevice::recvDoneCallback(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data,
                                      void *arg) {
    BaseType_t high_task_wakeup = pdFALSE;
    auto receive_queue = (QueueHandle_t) arg;
    xQueueSendFromISR(receive_queue, evt_data, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

esp_err_t I2cSlaveDevice::setup(i2c_port_num_t i2c_port, gpio_num_t sda_io_num, gpio_num_t scl_io_num) {
    for (size_t idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        _rx_queue[idx] = xQueueCreate(CONFIG_I2C_RX_QUEUE_SIZE, sizeof(ExchangeMessage));
        assert(_rx_queue[idx] != nullptr);

        _tx_queue[idx] = xQueueCreate(CONFIG_I2C_TX_QUEUE_SIZE, sizeof(ExchangeMessage));
        assert(_tx_queue[idx] != nullptr);
    }

    i2c_slave_config_t config = {
        .i2c_port = 0,
        .sda_io_num = sda_io_num,
        .scl_io_num = scl_io_num,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .send_buf_depth = 256,
        .slave_addr = I2C_DEVICE_ADDRESS,
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .flags{
            .access_ram_en = false,
        }
    };

    ESP_ERROR_CHECK(i2c_new_slave_device(&config, &_device));


    _dataQueue = xQueueCreate(1, sizeof(i2c_slave_rx_done_event_data_t));
    i2c_slave_event_callbacks_t callbacks{
        .on_recv_done = recvDoneCallback,
    };
    i2c_slave_register_event_callbacks(_device, &callbacks, _dataQueue);

    FreeRTOSTask::execute([this]() {
        exchange();
    }, "i2c-slave", 4096);

    return ESP_OK;
}

esp_err_t I2cSlaveDevice::writeData(const ExchangeMessage &buffer, TickType_t tick) {
    ExchangeMessage tx{};
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

esp_err_t I2cSlaveDevice::readData(ExchangeMessage &buffer, TickType_t tick) {
    while (true) {
        for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
            if (xQueueReceive(_rx_queue[idx], &buffer, 0)) {
                return ESP_OK;
            }
        }
        vTaskDelay(tick);
    }
}


void I2cSlaveDevice::exchange() {
    esp_logi(i2c_slave, "exchange running");
    i2c_slave_rx_done_event_data_t event{};
    while (true) {
        void *rx = malloc(I2C_RX_BUF_SIZE);
        memset(rx, 0, I2C_RX_BUF_SIZE);
        if (auto err = i2c_slave_receive(_device, (uint8_t *) rx, I2C_RX_BUF_SIZE); err == ESP_OK) {
            xQueueReceive(_dataQueue, &event, portMAX_DELAY);

            ExchangeMessage rxMsg{};
            unpackBuffer(rx, rxMsg);
            if (rxMsg.length) {
                esp_logi(esp_slave, "recv:  %d:%d, %d:%d:%d", rxMsg.if_type, rxMsg.if_num, rxMsg.length, rxMsg.payload_len, rxMsg.offset);
                if (ESP_OK != postRxBuffer(rxMsg)) {
                    free(rx);
                }
            } else {
                free(rx);
            }
        } else {
            esp_loge(i2c_slave, "failed to recv data, %s", esp_err_to_name(err));
            free(rx);
        }

        ExchangeMessage txMsg{};
        getNextTxBuffer(txMsg);
        if (txMsg.length) {
            esp_logi(esp_slave, "send: %d:%d:%d", txMsg.length, txMsg.payload_len, txMsg.offset);
        }
        if (auto err = i2c_slave_transmit(_device, (uint8_t *) txMsg.payload, I2C_RX_BUF_SIZE+1, portMAX_DELAY); err != ESP_OK) {
            esp_loge(i2c_slave, "failed to send data, %s", esp_err_to_name(err));
        }
        free(txMsg.payload);
    }
}

esp_err_t I2cSlaveDevice::getNextTxBuffer(ExchangeMessage &buf) const {
    BaseType_t ret = pdFAIL;

    /* Get or create new tx_buffer
     *	1. Check if SPI TX queue has pending buffers. Return if valid buffer is obtained.
     *	2. Create a new empty tx buffer and return */
    for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        if (ret = xQueueReceive(_tx_queue[idx], &buf, pdMS_TO_TICKS(50)); ret == pdTRUE) {
            break;
        }
    }

    if (ret == pdTRUE && buf.payload) {
        return ESP_OK;
    }

    buf.if_type = 0xF;
    buf.if_num = 0xF;
    buf.payload = malloc(I2C_RX_BUF_SIZE+1);
    buf.payload_len = I2C_RX_BUF_SIZE+1;
    buf.length = 0;
    buf.offset = sizeof(ExchangeHeader);
    memset(buf.payload, 0, I2C_RX_BUF_SIZE+1);
    memcpy(buf.payload, &buf, sizeof(ExchangeHeader));

    return ESP_OK;
}

esp_err_t I2cSlaveDevice::postRxBuffer(ExchangeMessage &rx_buf_handle) const {
    BaseType_t ret = pdFAIL;
    if (rx_buf_handle.if_type == ESP_INTERNAL_IF) {
        ret = xQueueSend(_rx_queue[PRIO_Q_HIGH], &rx_buf_handle, portMAX_DELAY);
    } else if (rx_buf_handle.if_type == ESP_HCI_IF) {
        ret = xQueueSend(_rx_queue[PRIO_Q_MID], &rx_buf_handle, portMAX_DELAY);
    } else {
        ret = xQueueSend(_rx_queue[PRIO_Q_LOW], &rx_buf_handle, portMAX_DELAY);
    }

    return ret == pdTRUE ? ESP_OK : ESP_FAIL;
}

void I2cSlaveDevice::destroy() {
}
