//
// Created by Ivan Kishchenko on 11/9/24.
//

#include "I2cSlaveDevice.h"

#ifdef CONFIG_EXCHANGE_BUS_I2C

#include <core/Logger.h>
#include <core/Task.h>

bool I2cSlaveDevice::recvDoneCallback(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data,
                                      void *arg) {
    BaseType_t high_task_wakeup = pdFALSE;
    auto receive_queue = static_cast<QueueHandle_t>(arg);
    xQueueSendFromISR(receive_queue, evt_data, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

I2cSlaveDevice::I2cSlaveDevice() {
    i2c_slave_config_t config = {
        .i2c_port = 0,
        .sda_io_num = pinSDA,
        .scl_io_num = pinSCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .send_buf_depth = CONFIG_EXCHANGE_BUS_BUFFER*2,
        .slave_addr = CONFIG_EXCHANGE_BUS_I2C_DEVICE_ADDR,
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
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(_device, &callbacks, _dataQueue));

    FreeRTOSTask::execute([this]() {
        exchange();
    }, "i2c-slave", 4096);
}

void I2cSlaveDevice::exchange() {
    esp_logi(i2c_slave, "exchange running");
    i2c_slave_rx_done_event_data_t event{};
    while (true) {
        void *rx = malloc(CONFIG_EXCHANGE_BUS_BUFFER);
        memset(rx, 0, CONFIG_EXCHANGE_BUS_BUFFER);
        if (auto err = i2c_slave_receive(_device, (uint8_t *) rx, CONFIG_EXCHANGE_BUS_BUFFER); err == ESP_OK) {
            xQueueReceive(_dataQueue, &event, portMAX_DELAY);

            auto* hdr = static_cast<ExchangeHeader*>(rx);
            if ((hdr->if_type == 0x0f && hdr->if_num == 0x0f) ||hdr->checksum == 0xffff) {
                free(rx);
                continue;
            }

            ExchangeMessage rxBuf{};
            if (ESP_OK != unpackBuffer(event.buffer, rxBuf)) {
                free(rx);
                continue;
            }
            esp_logi(i2c_slave, "recv: %d:%d:%d", rxBuf.length, rxBuf.payload_len, rxBuf.offset);
            if (ESP_OK != postRxBuffer(rxBuf)) {
                free(rx);
            }
        } else {
            esp_loge(i2c_slave, "failed to recv data, %s", esp_err_to_name(err));
            free(rx);
        }

        ExchangeMessage txMsg{};
        if (ESP_OK == getNextTxBuffer(txMsg)) {
            if (txMsg.length) {
                esp_logi(i2c_slave, "send: %d:%d:%d", txMsg.length, txMsg.payload_len, txMsg.offset);
            }
            if (auto err = i2c_slave_transmit(_device, (uint8_t *) txMsg.payload, CONFIG_EXCHANGE_BUS_BUFFER+1, portMAX_DELAY); err != ESP_OK) {
                esp_loge(i2c_slave, "failed to send data, %s", esp_err_to_name(err));
            }
            free(txMsg.payload);
        }
    }
}

#endif