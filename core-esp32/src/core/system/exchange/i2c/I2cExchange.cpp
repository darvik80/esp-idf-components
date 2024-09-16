//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "I2cExchange.h"

#include <esp_timer.h>
#include <core/Task.h>

#include "core/system/exchange/i2c/I2cMasterDevice.h"
#include "core/system/exchange/i2c/I2cSlaveDevice.h"

void fromJson(cJSON *json, I2cExchangeProperties &props) {
    if (json->type == cJSON_Object) {
        cJSON *item = json->child;
        while (item) {
            //             if (0 == strcmp(item->string, "device") && item->type == cJSON_String) {
            //                 if (0 == strcmp(item->valuestring, "SPI1")) {
            //                     props.device = SPI1_HOST;
            //                 } else if (0 == strcmp(item->valuestring, "SPI2")) {
            //                     props.device = SPI2_HOST;
            // #if SOC_SPI_PERIPH_NUM > 2
            //                 }else if (0 == strcmp(item->valuestring, "SPI3")) {
            //                     props.device = SPI3_HOST;
            // #endif
            //                 }
            //             } else
            if (!strcmp(item->string, "mode") && item->type == cJSON_String) {
                props.mode = 0 == strcmp(item->valuestring, "MASTER") ? I2C_MODE_MASTER : I2C_MODE_SLAVE;
            }

            item = item->next;
        }
    }
}

I2cExchange::I2cExchange(Registry &registry) : TService(registry) {
    registry.getPropsLoader().addReader("i2c", defaultPropertiesReader<I2cExchangeProperties>);
    registry.getPropsLoader().addConsumer(this);
}

void I2cExchange::setup() {
}

void I2cExchange::process(const ExchangeMessage &buffer) {
    if (buffer.payload_len && buffer.payload_len <= I2C_RX_BUF_SIZE) {
        esp_logi(spi, "Received Msg: %d, %d, %d, %s", buffer.payload_len, buffer.length, buffer.offset,
                 (char*)(buffer.payload + buffer.offset));
        std::string_view msg((char *) (buffer.payload + buffer.offset));
        if (msg.starts_with("ping")) {
            std::string ping = "pong";
            ExchangeMessage buf{
                ExchangeHeader{
                    .if_type = ESP_INTERNAL_IF,
                    .if_num = 0x02,
                    .pkt_type = PACKET_TYPE_COMMAND_RESPONSE,
                },
            };
            buf.payload = (void *) ping.data();
            buf.payload_len = ping.size() + 1;
            buf.length = buf.payload_len;

            esp_logi(spi, "Send: %s", ping.c_str());
            _device->writeData(buf);
        }
    }
}

void I2cExchange::apply(const I2cExchangeProperties &props) {
    if (props.mode == I2C_MODE_MASTER) {
        esp_logi(spi, "run %s, master mode", getServiceName().data());
        _device = new I2cMasterDevice();
    } else {
        esp_logi(spi, "run %s, slave mode", getServiceName().data());
        _device = new I2cSlaveDevice();
    }

    _device->setup(-1, I2C_SDA_IO, I2C_SCL_IO);

    FreeRTOSTask::execute(
        [this] {
            ExchangeMessage buf_handle{};

            for (;;) {
                if (ESP_FAIL == _device->readData(buf_handle)) {
                    continue;
                }

                process(buf_handle);
                free(buf_handle.payload);
            }
        },
        getServiceName(), 4096
    );

    if (props.mode == I2C_MODE_SLAVE) {
        FreeRTOSTask::execute([this, props]() {
            vTaskDelay(pdMS_TO_TICKS(10000));
            while (true) {
                std::string ping = "ping";
                ExchangeMessage buf{
                    ExchangeHeader{
                        .if_type = ESP_INTERNAL_IF,
                        .if_num = 0x08,
                        .pkt_type = PACKET_TYPE_COMMAND_REQUEST,
                    },
                };
                buf.payload = (void *) ping.data();
                buf.payload_len = ping.size() + 1;
                buf.length = buf.payload_len;
                esp_logi(spi, "Send: %s", ping.c_str());
                _device->writeData(buf);
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
        }, "tester", 4096);
    }
}
