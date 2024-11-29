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
        uart_driver_install(_port, CONFIG_EXCHANGE_BUS_BUFFER, CONFIG_EXCHANGE_BUS_BUFFER, 10, &_rxQueue,
            intr_alloc_flags));

    ESP_ERROR_CHECK(uart_flush_input(_port));
    FreeRTOSTask::execute([this]() {
        rxTask();
    }, "uart-rx", 4096);

    FreeRTOSTask::execute([this]() {
        txTask();
    }, "uart-tx", 4096);
}

enum rx_state_t {
    UART_RX_HEADER,
    UART_RX_HEADER_FRAGMENTED,
    UART_RX_PAYLOAD,
};

void UartDevice::rxTask() {
    esp_logi(uart, "rxTask running");

    size_t expectingSize = sizeof(transfer_header_t);
    rx_state_t state = UART_RX_HEADER;
    uart_flush_input(_port);
    xQueueReset(_rxQueue);

    uint8_t *msg = nullptr;
    while (true) {
        // send data
        uart_event_t event{};
        if (xQueueReceive(_rxQueue, (void *) &event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: {
                    if (!msg) {
                        msg = (uint8_t *) malloc(CONFIG_EXCHANGE_BUS_BUFFER);
                        memset(msg, 0, CONFIG_EXCHANGE_BUS_BUFFER);
                    }

                    esp_logd(uart, "rxTask received data: %d", event.size);
                    size_t size;
                    uart_get_buffered_data_len(_port, &size);
                    if (size < expectingSize) {
                        break;
                    }

                    while (size) {
                        switch (state) {
                            case UART_RX_HEADER:
                            case UART_RX_HEADER_FRAGMENTED: {
                                while (size && state != UART_RX_PAYLOAD) {
                                    if (state == UART_RX_HEADER) {
                                        uart_read_bytes(_port, msg, sizeof(transfer_header_t), portMAX_DELAY);
                                        size -= sizeof(transfer_header_t);
                                    } else {
                                        memmove(&msg[0], &msg[1], sizeof(transfer_header_t) - 1);
                                        uart_read_bytes(_port, &msg[sizeof(transfer_header_t) - 1], sizeof(uint8_t),
                                                        portMAX_DELAY);
                                        size--;
                                    }

                                    //ESP_LOG_BUFFER_HEX("uart", msg, sizeof(transfer_header_t));

                                    auto *ptr = reinterpret_cast<transfer_header_t *>(msg);
                                    if (ptr->stx != STX_BIT || ptr->etx != ETX_BIT) {
                                        state = UART_RX_HEADER_FRAGMENTED;
                                        esp_logd(uart, "detected fragmented hdr...");
                                    } else {
                                        state = UART_RX_PAYLOAD;
                                        expectingSize = ptr->payload_len;
                                        esp_logd(uart, "detected hdr, switch to read payload...");
                                    }
                                }
                            }
                                [[fallthrough]];
                            case UART_RX_PAYLOAD: {
                                if (size < expectingSize) {
                                    break;
                                }
                                uart_read_bytes(_port, msg + sizeof(transfer_header_t), expectingSize, portMAX_DELAY);
                                //ESP_LOG_BUFFER_HEX("uart", msg + sizeof(transfer_header_t), expectingSize);
                                esp_logd(uart, "got payload: %d...", expectingSize);

                                exchange_message_t rx_buf{};
                                size -= expectingSize;
                                if (ESP_OK != unpackBuffer(msg, rx_buf)) {
                                    esp_logw(uart, "drop msg, can't unpack message");
                                } else if (ESP_OK != postRxBuffer(rx_buf)) {
                                    esp_logw(uart, "ignore msg, can't post message");
                                } else {
                                    // free buffer when will be processed
                                    msg = nullptr;
                                }
                                state = UART_RX_HEADER;
                                expectingSize = sizeof(transfer_header_t);
                            }
                        }
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
    esp_logi(esp_now, "txTask running");
    while (true) {
        // send data
        exchange_message_t tx_buf{};
        if (ESP_OK == getNextTxBuffer(tx_buf)) {
            uart_write_bytes(_port, tx_buf.payload, tx_buf.length);
            free(tx_buf.payload);
        }
    }
}

#endif
