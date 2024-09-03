//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "SpiExchange.h"

#include <esp_timer.h>
#include <core/Task.h>
#include <driver/gpio.h>

#include "SpiMasterDevice.h"
#include "SpiSlaveDevice.h"


void fromJson(cJSON *json, SpiExchangeProperties &props) {
    if (json->type == cJSON_Object) {
        cJSON *item = json->child;
        while (item) {
            if (0 == strcmp(item->string, "device") && item->type == cJSON_String) {
                if (0 == strcmp(item->valuestring, "SPI1")) {
                    props.device = SPI1_HOST;
                } else if (0 == strcmp(item->valuestring, "SPI2")) {
                    props.device = SPI2_HOST;
#if SOC_SPI_PERIPH_NUM > 2
                }else if (0 == strcmp(item->valuestring, "SPI2")) {
                    props.device = SPI3_HOST;
#endif
                }
            } else if (!strcmp(item->string, "mode") && item->type == cJSON_String) {
                props.mode = (0 == strcmp(item->valuestring, "MASTER")) ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
            }

            item = item->next;
        }
    }
}

SpiExchange::SpiExchange(Registry &registry) : TService(registry) {
    registry.getPropsLoader().addReader("spi", defaultPropertiesReader<SpiExchangeProperties>);
    registry.getPropsLoader().addConsumer(this);
}

void SpiExchange::setup() {
}

void SpiExchange::process(const SpiMessage &buffer) {
    if (buffer.length && buffer.offset) {
        esp_logi(spi, "Received Msg: %d, %d, %d, %s", buffer.payload_len, buffer.length, buffer.offset, (char*)(buffer.payload + buffer.offset));
        if (buffer.payload_len && buffer.payload_len < RX_BUF_SIZE) {
            std::string_view msg((char*)(buffer.payload + buffer.offset));
            if (msg.starts_with("ping")) {
                std::string ping = "pong from master: ";
                ping += msg;
                esp_logi(spi, "Send Msg: %s", ping.c_str());
                SpiMessage buf{
                    SpiHeader{
                        .if_type = ESP_INTERNAL_IF,
                        .pkt_type = PACKET_TYPE_COMMAND_RESPONSE,
                    },
                };
                buf.payload = (void *) ping.data();
                buf.payload_len = ping.size() + 1;
                buf.length = buf.payload_len;

                //TODO force send data to slave
                _spi->writeData(&buf);
            }
        }
    }
}

void SpiExchange::apply(const SpiExchangeProperties &props) {
    if (props.mode == SPI_MODE_MASTER) {
        esp_logi(spi, "run %s, master mode", getServiceName().data());
        _spi = new SpiMasterDevice(props.device);
    } else {
        esp_logi(spi, "run %s, slave mode", getServiceName().data());
        _spi = new SpiSlaveDevice(props.device);

        std::string msg = "Init message from SLAVE";
        SpiMessage buf{
            SpiHeader{
                .if_type = ESP_INTERNAL_IF,
                .pkt_type = PACKET_TYPE_COMMAND_REQUEST,
            },
        };
        buf.payload = (void *) msg.data();
        buf.payload_len = msg.size() + 1;
        buf.length = buf.payload_len;
        esp_logi(spi, "Slave send init message");
        _spi->writeData(&buf);
    }

    _spi->setup();

    FreeRTOSTask::execute(
        [this] {
            SpiMessage buf_handle{};

            for (;;) {
                if (ESP_FAIL == _spi->readData(&buf_handle)) {
                    //usleep(10 * 1000);
                    continue;
                }

                process(buf_handle);
                free(buf_handle.payload);
            }
        },
        getServiceName(), 4096
    );

    if (props.mode == SPI_MODE_SLAVE) {
        FreeRTOSTask::execute([this, props]() {
            int count{0};
            vTaskDelay(pdMS_TO_TICKS(10000));
            while (true) {
                std::string ping = "ping: " + std::to_string(++count);
                SpiMessage buf{
                    SpiHeader{
                        .if_type = ESP_INTERNAL_IF,
                        .pkt_type = PACKET_TYPE_COMMAND_REQUEST,
                    },
                };
                buf.payload = (void *) ping.data();
                buf.payload_len = ping.size() + 1;
                buf.length = buf.payload_len;
                esp_logi(spi, "Send: %s", ping.c_str());
                _spi->writeData(&buf);
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
        }, "tester", 4096);
    }
    // FreeRTOSTask::execute([this, props]() {
    //     int h = gpio_get_level(PIN_NUM_MASTER_HANDSHAKE);
    //     int r = gpio_get_level(PIN_NUM_MASTER_DATA_READY);
    //     esp_logi(spi_master, "handshake: %d, data_ready: %d", h, r);
    //     while (true) {
    //         int hn = gpio_get_level(PIN_NUM_MASTER_HANDSHAKE);
    //         int rn = gpio_get_level(PIN_NUM_MASTER_DATA_READY);
    //         if (hn != h || rn != r) {
    //             h = hn; r = rn;
    //             esp_logi(spi_master, "handshake: %d, data_ready: %d", h, r);
    //         }
    //         vTaskDelay(1);
    //     }
    // }, "tester", 4096);
}
