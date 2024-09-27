//
// Created by Ivan Kishchenko on 19/9/24.
//


#include "UartDevice.h"

#ifdef CONFIG_EXCHANGE_BUS_UART

#include <core/Task.h>
#include <driver/uart.h>
#include "core/Logger.h"

UartDevice::UartDevice(uart_port_t port, int baudRate) {
    _port = port;

    if (uart_is_driver_installed(_port)) {
        uart_driver_delete(_port);
    }

    /* Configure parameters of an UART driver,
         * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = baudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_param_config(_port, &uart_config));
    ESP_ERROR_CHECK(
        uart_set_pin(_port, CONFIG_EXCHANGE_BUS_UART_TX_PIN, CONFIG_EXCHANGE_BUS_UART_RX_PIN, UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(
        uart_driver_install(_port, CONFIG_EXCHANGE_BUS_BUFFER,CONFIG_EXCHANGE_BUS_BUFFER, 10, &_rxQueue,
            intr_alloc_flags));

    ESP_ERROR_CHECK(uart_flush_input(_port));
    FreeRTOSTask::execute([this]() {
        rxTask();
    }, "uart-rx", 4096);

    FreeRTOSTask::execute([this]() {
        txTask();
    }, "uart-tx", 4096);
}

void UartDevice::rxTask() {
    esp_logi(uart, "rxTask running");
    uart_flush_input(_port);
    xQueueReset(_rxQueue);

    while (true) {
        // send data
        uart_event_t event{};
        if (xQueueReceive(_rxQueue, (void *) &event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: {
                    ExchangeHeader hdr{};
                    if (event.size >= sizeof(ExchangeHeader)) {
                        uart_read_bytes(_port, &hdr, sizeof(ExchangeHeader), portMAX_DELAY);
                    }
                    // filter noise
                    if (event.size > CONFIG_EXCHANGE_BUS_BUFFER || hdr.stx != STX_HDR || hdr.payload_len >
                        CONFIG_EXCHANGE_BUS_BUFFER) {
                        esp_logd(uart, "drop rx-data, invalid msg, event-size: %d, stx: %04x, payload:", event.size,
                                 hdr.stx, hdr.payload_len);
                        uart_flush_input(_port);
                        xQueueReset(_rxQueue);
                        break;
                    }
                    auto *rx = static_cast<uint8_t *>(malloc(CONFIG_EXCHANGE_BUS_BUFFER));
                    memcpy(rx, &hdr, sizeof(ExchangeHeader));
                    if (hdr.payload_len > sizeof(ExchangeHeader)) {
                        size_t len = std::min(hdr.payload_len - sizeof(ExchangeHeader),
                                              event.size - sizeof(ExchangeHeader));
                        if (len) {
                            uart_read_bytes(_port, rx + sizeof(ExchangeHeader), len, portMAX_DELAY);
                        }
                    }

                    ExchangeMessage rx_buf{};
                    if (ESP_OK != unpackBuffer(rx, rx_buf)) {
                        esp_logw(uart, "drop msg, can't unpack message");
                        free(rx);
                    } else if (ESP_OK != postRxBuffer(rx_buf)) {
                        esp_logw(uart, "ignore msg, can't post message");
                        free(rx);
                    }
                }
                break;
                default: {
                    esp_logd(uart, "uart[%d] event: %d", _port, event.type);
                    uart_flush_input(_port);
                    xQueueReset(_rxQueue);
                }
                break;
            }
        }
    }
}

void UartDevice::txTask() {
    esp_logi(i2c_master, "txTask running");
    while (true) {
        // send data
        ExchangeMessage tx_buf{};
        if (ESP_OK == getNextTxBuffer(tx_buf)) {
            uart_write_bytes(_port, tx_buf.payload, tx_buf.payload_len);
            free(tx_buf.payload);
        }
    }
}

#endif