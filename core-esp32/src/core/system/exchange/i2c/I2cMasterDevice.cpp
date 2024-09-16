//
// Created by Ivan Kishchenko on 11/9/24.
//

#include "I2cMasterDevice.h"

#include <core/Logger.h>
#include <core/Task.h>


bool IRAM_ATTR master_recv_done(i2c_master_dev_handle_t i2c_dev, const i2c_master_event_data_t *evt_data, void *arg) {
    BaseType_t high_task_wakeup = pdFALSE;
    auto receive_queue = (QueueHandle_t) arg;
    xQueueSendFromISR(receive_queue, evt_data, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

esp_err_t I2cMasterDevice::setup(i2c_port_num_t i2c_port, gpio_num_t sda_io_num, gpio_num_t scl_io_num) {
    for (size_t idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        _rx_queue[idx] = xQueueCreate(CONFIG_I2C_RX_QUEUE_SIZE, sizeof(ExchangeMessage));
        assert(_rx_queue[idx] != nullptr);

        _tx_queue[idx] = xQueueCreate(CONFIG_I2C_TX_QUEUE_SIZE, sizeof(ExchangeMessage));
        assert(_tx_queue[idx] != nullptr);
    }

    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = 0,
        .sda_io_num = sda_io_num,
        .scl_io_num = scl_io_num,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .trans_queue_depth = 7,
        .flags{
            .enable_internal_pullup = true,
        }
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &_bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_DEVICE_ADDRESS,
        .scl_speed_hz = 400000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(_bus, &dev_cfg, &_device));

    _dataQueue = xQueueCreate(1, sizeof(i2c_master_event_data_t));
    i2c_master_event_callbacks_t callbacks{
        .on_trans_done = master_recv_done
    };

    ESP_ERROR_CHECK(i2c_master_register_event_callbacks(_device, &callbacks, _dataQueue));

    //ESP_ERROR_CHECK(i2c_master_probe(_bus, I2C_DEVICE_ADDRESS, portMAX_DELAY));

    FreeRTOSTask::execute([this]() {
        exchange();
    }, "i2c-master", 4096);

    return ESP_OK;
}

esp_err_t I2cMasterDevice::writeData(const ExchangeMessage &buffer, TickType_t tick) {
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

esp_err_t I2cMasterDevice::readData(ExchangeMessage &buffer, TickType_t tick) {
    while (true) {
        for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
            if (xQueueReceive(_rx_queue[idx], &buffer, tick)) {
                return ESP_OK;
            }
        }
        vTaskDelay(1);
    }
}

void I2cMasterDevice::exchange() {
    esp_logi(i2c_master, "exchange running");
    // sync
    while (true) {
        // send data
        ExchangeMessage tx_buf{}, rx_buf{};
        if (ESP_OK == getNextTxBuffer(tx_buf)) {
            // esp_logi(esp_master, "send:  %d:%d, %d:%d:%d", tx_buf.if_type, tx_buf.if_num, tx_buf.length,
            //          tx_buf.payload_len, tx_buf.offset);
            auto err = i2c_master_transmit(
                _device,
                (uint8_t *) tx_buf.payload, I2C_RX_BUF_SIZE,
                -1
            );

            if (err != ESP_OK) {
                esp_loge(i2c_master, "failed to send data, %s", esp_err_to_name(err));
            }
            free(tx_buf.payload);

            auto *rx = malloc(I2C_RX_BUF_SIZE);
            memset(rx, 0, I2C_RX_BUF_SIZE);
            err = i2c_master_receive(_device, (uint8_t *) rx, I2C_RX_BUF_SIZE, -1);
            if (err != ESP_OK) {
                esp_loge(i2c_master, "failed to recv data, %s", esp_err_to_name(err));
            } else {
                i2c_master_event_data_t event{};
                auto res = xQueueReceive(_dataQueue, &event, portMAX_DELAY);
                unpackBuffer(rx, rx_buf);
                if (rx_buf.length) {
                    esp_logi(esp_master, "recv:  %d, %02x:%02x, %d:%d:%d", res, rx_buf.if_type, rx_buf.if_num, rx_buf.length,rx_buf.payload_len, rx_buf.offset);
                    if (ESP_OK != postRxBuffer(rx_buf)) {
                        free(rx);
                    }
                } else {
                    free(rx);
                }
            }
        }

        //i2c_master_bus_wait_all_done(_bus, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

esp_err_t I2cMasterDevice::getNextTxBuffer(ExchangeMessage &buf) const {
    BaseType_t ret = pdFAIL;

    /* Get or create new tx_buffer
     *	1. Check if SPI TX queue has pending buffers. Return if valid buffer is obtained.
     *	2. Create a new empty tx buffer and return */
    for (int idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
        if (ret = xQueueReceive(_tx_queue[idx], &buf, 0); ret == pdTRUE) {
            break;
        }
    }

    if (ret == pdTRUE && buf.payload) {
        return ESP_OK;
    }

    buf.if_type = 0xF;
    buf.if_num = 0xF;
    buf.payload = malloc(I2C_RX_BUF_SIZE);
    buf.payload_len = I2C_RX_BUF_SIZE;
    buf.length = 0;
    buf.offset = sizeof(ExchangeHeader);
    memset(buf.payload, 0, I2C_RX_BUF_SIZE);
    memcpy(buf.payload, &buf, sizeof(ExchangeHeader));

    return ESP_OK;
}

esp_err_t I2cMasterDevice::postRxBuffer(ExchangeMessage &rx_buf_handle) const {
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

void I2cMasterDevice::destroy() {
}
