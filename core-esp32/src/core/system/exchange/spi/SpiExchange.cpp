//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "SpiExchange.h"

#ifdef CONFIG_EXCHANGE_BUS_SPI

#include <esp_timer.h>
#include <core/Task.h>
#include <driver/gpio.h>

#include "SpiMasterDevice.h"
#include "SpiSlaveDevice.h"


void fromJson(cJSON *json, SpiExchangeProperties &props) {
    if (json->type == cJSON_Object) {
        cJSON *item = json->child;
        while (item) {
            if (!strcmp(item->string, "mode") && item->type == cJSON_String) {
                props.mode = (0 == strcmp(item->valuestring, "MASTER")) ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
            }

            item = item->next;
        }
    }
}

SpiExchange::SpiExchange(Registry &registry) : AbstractExchange(registry) {
    registry.getPropsLoader().addReader("spi", defaultPropertiesReader<SpiExchangeProperties>);
    registry.getPropsLoader().addConsumer(this);
}

void SpiExchange::apply(const SpiExchangeProperties &props) {
    if (props.mode == SPI_MODE_MASTER) {
        esp_logi(spi, "run %s, master mode", getServiceName().data());
        _device = new SpiMasterDevice();
    } else {
        esp_logi(spi, "run %s, slave mode", getServiceName().data());
        _device = new SpiSlaveDevice();

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
        _device->writeData(buf);
    }
}

#endif
