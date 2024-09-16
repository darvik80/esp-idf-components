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
                }else if (0 == strcmp(item->valuestring, "SPI3")) {
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

void SpiExchange::process(const ExchangeMessage &buffer) {
    if (buffer.payload_len && buffer.payload_len < RX_BUF_SIZE) {
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

            //TODO force send data to slave
            _spi->writeData(&buf);
        }
    }
    onMessage(buffer);
}

void SpiExchange::apply(const SpiExchangeProperties &props) {
    if (props.mode == SPI_MODE_MASTER) {
        esp_logi(spi, "run %s, master mode, spi: %d", getServiceName().data(), props.device+1);
        _spi = new SpiMasterDevice(props.device);
    } else {
        esp_logi(spi, "run %s, slave mode, spi: %d", getServiceName().data(), props.device+1);
        _spi = new SpiSlaveDevice(props.device);

        std::string msg = "Init message from SLAVE: ";
        msg += std::to_string(esp_timer_get_time());
        ExchangeMessage buf{
            ExchangeHeader{
                .if_type = ESP_INTERNAL_IF,
                .if_num = 0x04,
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
            ExchangeMessage buf_handle{};

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
                _spi->writeData(&buf);
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
        }, "tester", 4096);
    }
}

void SpiExchange::send(const ExchangeMessage &msg) {
    _spi->writeData(&msg);
}

void SpiExchange::onMessage(const ExchangeMessage &msg) {

}
