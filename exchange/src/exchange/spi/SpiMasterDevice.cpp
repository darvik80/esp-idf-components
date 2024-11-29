//
// Created by Ivan Kishchenko on 29/8/24.
//

#include "SpiMasterDevice.h"

#ifdef CONFIG_EXCHANGE_BUS_SPI

#include <core/Task.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#define HANDSHAKE_BIT 0x01
#define DATA_READY_BIT 0x02

void IRAM_ATTR SpiMasterDevice::gpio_handshake_isr_handler(void *arg) {
    // //Sometimes due to interference or ringing or something, we get two irqs after eachother. This is solved by
    // //looking at the time between interrupts and refusing any interrupt too close to another one.
    // static uint32_t lasthandshaketime_us;
    // uint32_t currtime_us = esp_timer_get_time();
    // uint32_t diff = currtime_us - lasthandshaketime_us;
    // if (diff < 1000) {
    //     return; //ignore everything <1ms after an earlier irq
    // }
    // lasthandshaketime_us = currtime_us;

    //Give the semaphore.
    auto *self = static_cast<SpiMasterDevice *>(arg);
    BaseType_t mustYield = false;
    xEventGroupSetBitsFromISR(self->_events, HANDSHAKE_BIT, &mustYield);
    if (mustYield) {
        portYIELD_FROM_ISR(mustYield);
    }
}

void IRAM_ATTR SpiMasterDevice::gpio_ready_data_isr_handler(void *arg) {
    // //Sometimes due to interference or ringing or something, we get two irqs after eachother. This is solved by
    // //looking at the time between interrupts and refusing any interrupt too close to another one.
    // static uint32_t lasthandshaketime_us;
    // uint32_t currtime_us = esp_timer_get_time();
    // uint32_t diff = currtime_us - lasthandshaketime_us;
    // if (diff < 1000) {
    //     return; //ignore everything <1ms after an earlier irq
    // }
    // lasthandshaketime_us = currtime_us;
    //
    //Give the semaphore.
    auto *self = static_cast<SpiMasterDevice *>(arg);
    BaseType_t mustYield = false;
    xEventGroupSetBitsFromISR(self->_events, DATA_READY_BIT, &mustYield);
    if (mustYield) {
        portYIELD_FROM_ISR(mustYield);
    }
}

void SpiMasterDevice::exchange() {

    xEventGroupSetBits(_events, HANDSHAKE_BIT | DATA_READY_BIT);
    while (true) {
        EventBits_t res = xEventGroupWaitBits(_events, HANDSHAKE_BIT | DATA_READY_BIT, true, true, pdMS_TO_TICKS(50));
        if ((res & (HANDSHAKE_BIT | DATA_READY_BIT)) != (HANDSHAKE_BIT | DATA_READY_BIT)) {
            auto handshake = gpio_get_level(pinHandshake);
            auto data_ready = gpio_get_level(pinDataReady);
            esp_logd(spi_master, "[t]: spi waiting timeout: %d, %d", handshake, data_ready);
            if (!handshake) {
                esp_logd(spi_master, "[t]: slave not ready...");
                continue;
            }

            if (!data_ready) {
                esp_logd(spi_master, "[t]: slave is ready but no data...");
                continue;
            }

            esp_logw(
                spi_master, "[t]: slave is ready and has data: %.02x %d:%d, %d:%d...", (int)res,
                handshake, (int)res & HANDSHAKE_BIT, data_ready, (int)res & DATA_READY_BIT
            );
        } else {
            esp_logd(spi_master, "[n]: slave is ready and has data...");
        }

        exchange_message_t txBuf{}, rxBuf{};
        if (ESP_OK != getNextTxBuffer(txBuf)) {
            continue;
        }

        spi_transaction_t spi_trans{
            .length = CONFIG_EXCHANGE_BUS_BUFFER * SPI_BITS_PER_WORD,
            .tx_buffer = txBuf.payload,
            .rx_buffer = heap_caps_malloc(CONFIG_EXCHANGE_BUS_BUFFER, MALLOC_CAP_DMA),
        };
        memset(spi_trans.rx_buffer, 0, CONFIG_EXCHANGE_BUS_BUFFER);

        gpio_set_level(pinCS, 0);
        // Lower the CS' line to select the slave
        if (auto err = spi_device_transmit(_spiDevice, &spi_trans); err != ESP_OK) {
            esp_loge(spi_master, "spi transmit error, err: 0x%x (%s)", err, esp_err_to_name(err));
            free(spi_trans.rx_buffer);
            free((void *) spi_trans.tx_buffer);
        } else {
            /* Free any tx buffer, data is not relevant anymore */
            if (spi_trans.tx_buffer) {
                free(const_cast<void *>(spi_trans.tx_buffer));
            }

            /* Process received data */
            if (spi_trans.rx_buffer) {
                if (ESP_OK != unpackBuffer(spi_trans.rx_buffer, rxBuf)) {
                    free(spi_trans.rx_buffer);
                    continue;
                }
                if (rxBuf.if_type == 0x0f && rxBuf.if_num == 0x0f) {
                    free(spi_trans.rx_buffer);
                    continue;
                }

                if (ESP_OK != postRxBuffer(rxBuf)) {
                    free(spi_trans.rx_buffer);
                }
            }
        }
        gpio_set_level(pinCS, 1);
        // if (gpio_get_level(pinDataReady)) {
        //     xEventGroupSetBits(_events, DATA_READY_BIT);
        // }
    }
}

SpiMasterDevice::SpiMasterDevice() {
    auto spi = static_cast<spi_host_device_t>(CONFIG_EXCHANGE_BUS_SPI_CHANNEL);
    gpio_set_direction(static_cast<gpio_num_t>(CONFIG_EXCHANGE_BUS_SPI_CS_PIN), GPIO_MODE_OUTPUT);
    // Setting the CS' pin to work in OUTPUT mode

    spi_bus_config_t buscfg = {
        // Provide details to the SPI_bus_sturcture of pins and maximum data size
        .mosi_io_num = CONFIG_EXCHANGE_BUS_SPI_MOSI_PIN,
        .miso_io_num = CONFIG_EXCHANGE_BUS_SPI_MISO_PIN,
        .sclk_io_num = CONFIG_EXCHANGE_BUS_SPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
    };

    spi_device_interface_config_t devcfg = {
        .mode = 0, // SPI mode 0: CPOL:-0 and CPHA:-0
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // Clock out at 12 MHz
        .spics_io_num = CONFIG_EXCHANGE_BUS_SPI_CS_PIN,
        // This field is used to specify the GPIO pin that is to be used as CS'
        .queue_size = 7, // We want to be able to queue 7 transactions at a time
    };

    //GPIO config for the handshake line.
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(pinHandshake),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    /* Configuration for data_ready line */
    gpio_config_t io_conf_ready = {
        .pin_bit_mask = BIT64(pinDataReady),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    gpio_config(&io_conf);
    gpio_config(&io_conf_ready);

    gpio_install_isr_service(0);
    gpio_set_intr_type(pinHandshake, GPIO_INTR_POSEDGE);
    ESP_ERROR_CHECK(gpio_isr_handler_add(pinHandshake, gpio_handshake_isr_handler,this));

    gpio_set_intr_type(pinDataReady, GPIO_INTR_POSEDGE);
    ESP_ERROR_CHECK(gpio_isr_handler_add(pinDataReady, gpio_ready_data_isr_handler, this));

    ESP_ERROR_CHECK(spi_bus_initialize(spi, &buscfg, SPI_DMA_CH_AUTO)); // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(spi, &devcfg, &_spiDevice)); // Attach the Slave device to the SPI bus

    _events = xEventGroupCreate();

    FreeRTOSTask::execute([this]() {
        exchange();
    }, "spi-device-task", SPI_TASK_DEFAULT_STACK_SIZE, SPI_TASK_DEFAULT_PRIO);

    usleep(500);
}

esp_err_t SpiMasterDevice::getNextTxBuffer(exchange_message_t &txBuf) {
    if (ESP_OK == ExchangeDevice::getNextTxBuffer(txBuf)) {
        if (hasData()) {
            xEventGroupSetBits(_events, DATA_READY_BIT);
        }
        return ESP_OK;
    }

    exchange_message_t dummy{
        .if_type = 0xF,
        .if_num = 0xF,
    };
    return packBuffer(dummy, txBuf);
}

esp_err_t SpiMasterDevice::writeData(const exchange_message_t &buffer, TickType_t tick) {
    if (auto err = ExchangeDevice::writeData(buffer, tick); err != ESP_OK) {
        return err;
    }

    xEventGroupSetBits(_events, DATA_READY_BIT);

    return ESP_OK;
}

SpiMasterDevice::~SpiMasterDevice() {
    spi_bus_remove_device(_spiDevice);
    spi_bus_free(spi);
}

#endif
