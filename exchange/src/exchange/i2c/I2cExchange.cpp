//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "I2cExchange.h"

#ifdef CONFIG_EXCHANGE_BUS_I2C

#include <esp_timer.h>
#include <core/Task.h>

#include "I2cMasterDevice.h"
#include "I2cSlaveDevice.h"

void fromJson(cJSON *json, I2cExchangeProperties &props) {
    if (json->type == cJSON_Object) {
        cJSON *item = json->child;
        while (item) {
            if (!strcmp(item->string, "mode") && item->type == cJSON_String) {
                props.mode = 0 == strcmp(item->valuestring, "MASTER") ? I2C_MODE_MASTER : I2C_MODE_SLAVE;
            }

            item = item->next;
        }
    }
}

I2cExchange::I2cExchange(Registry &registry) : AbstractExchange(registry) {
    registry.getPropsLoader().addReader("i2c", defaultPropertiesReader<I2cExchangeProperties>);
    registry.getPropsLoader().addConsumer(this);
}

void I2cExchange::apply(const I2cExchangeProperties &props) {
    if (props.mode == I2C_MODE_MASTER) {
        esp_logi(spi, "run %s, master mode", getServiceName().data());
        _device = new I2cMasterDevice();
    } else {
        esp_logi(spi, "run %s, slave mode", getServiceName().data());
        _device = new I2cSlaveDevice();
    }

    FreeRTOSTask::execute(
        [this] {
            ExchangeMessage buf_handle{};

            for (;;) {
                if (ESP_FAIL == _device->readData(buf_handle)) {
                    continue;
                }

                onMessage(buf_handle);
                free(buf_handle.payload);
            }
        },
        getServiceName(), 4096
    );
}

#endif