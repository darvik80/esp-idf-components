//
// Created by Ivan Kishchenko on 11/9/24.
//

#include "I2cMasterDevice.h"

#ifdef CONFIG_EXCHANGE_BUS_I2C

#include <core/Logger.h>
#include <core/Task.h>

bool IRAM_ATTR master_recv_done(i2c_master_dev_handle_t i2c_dev, const i2c_master_event_data_t *evt_data, void *arg) {
    BaseType_t high_task_wakeup = pdFALSE;
    auto receive_queue = static_cast<QueueHandle_t>(arg);
    xQueueSendFromISR(receive_queue, evt_data, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

I2cMasterDevice::I2cMasterDevice() {
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = 0,
        .sda_io_num = pinSDA,
        .scl_io_num = pinSCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags{
            .enable_internal_pullup = true,
        }
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &_bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_EXCHANGE_BUS_I2C_DEVICE_ADDR,
        .scl_speed_hz = CONFIG_EXCHANGE_BUS_I2C_SCL_SPEED_FREQ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(_bus, &dev_cfg, &_device));

    ESP_ERROR_CHECK(i2c_master_probe(_bus, CONFIG_EXCHANGE_BUS_I2C_DEVICE_ADDR, -1));

    FreeRTOSTask::execute([this]() {
        exchange();
    }, "i2c-master", 4096);
}

void I2cMasterDevice::exchange() {
    esp_logi(i2c_master, "exchange running");
    // sync
    while (true) {
        // send data
        ExchangeMessage txBuf{}, rxBuf{};
        if (ESP_OK == getNextTxBuffer(txBuf)) {
            auto *rx = malloc(CONFIG_EXCHANGE_BUS_BUFFER);
            memset(rx, 0, CONFIG_EXCHANGE_BUS_BUFFER);

            auto err = i2c_master_transmit_receive(
                _device,
                (uint8_t *) txBuf.payload, CONFIG_EXCHANGE_BUS_BUFFER,
                (uint8_t *) rx, CONFIG_EXCHANGE_BUS_BUFFER,
                -1
            );
            free(txBuf.payload);

            if (err != ESP_OK) {
                esp_loge(i2c_master, "failed to transmit/receive data, %s", esp_err_to_name(err));
                free(rx);
            } else {
                auto* hdr = static_cast<ExchangeHeader*>(rx);
                if ((hdr->if_type == 0x0f && hdr->if_num == 0x0f) ||hdr->checksum == 0xffff) {
                    free(rx);
                    continue;
                }

                if (ESP_OK != unpackBuffer(rx, rxBuf)) {
                    free(rx);
                } else if (ESP_OK != postRxBuffer(rxBuf)) {
                    free(rx);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

#endif
