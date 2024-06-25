//
// Created by Ivan Kishchenko on 22/6/24.
//

#include <driver/uart.h>
#include <soc/gpio_num.h>
#include <iostream>
#include <driver/gpio.h>
#include "UartExchange.h"

UartExchange::UartExchange(Registry &registry) : TService(registry) {

}

void UartExchange::setup() {
    uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

   esp_logi(uart, "test test test");
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, GPIO_NUM_21,  GPIO_NUM_20, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024 * 2, 1024 * 2, 256, &_rxQueue, 0));

    xTaskCreate(rxTask, "uart-rx-task", 8192, this, 21, nullptr);
//    xTaskCreate(uartSendingTask, "uart_tx_task", 4096, NULL, 13, NULL);
    esp_logi(uart, "uart-exchange started");

//    gpio_set_level(GPIO_NUM_7, 0);
//    for (int idx = 0; idx < 100; idx++) {
//        std::string_view msg("Hello World!");
//        uart_write_bytes(UART_NUM_0, msg.data(), msg.size());
//        vTaskDelay(1000/portTICK_PERIOD_MS);
//    }
}

[[noreturn]] void UartExchange::rxTask() {
    uart_event_t event;
    std::array<char*, 1024> buf{};
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(_rxQueue, (void *) &event, (TickType_t) portMAX_DELAY)) {
            buf.fill(0);
            esp_logd(uart, "uart[%d] event:", UART_NUM_0);
            switch (event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    if (event.size) {
                        auto len = uart_read_bytes(UART_NUM_0, buf.data(), event.size, portMAX_DELAY);
                        if (0 == gpio_get_level(GPIO_NUM_7)) {
                            buf[len] = 0x00;
                            esp_logi(uart, "data: %s", (const char *) buf.data());
                        }
                    }

                    break;
                    //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    esp_logw(uart, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(_rxQueue);
                    break;
                    //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    esp_logw(uart, "ring buffer full");
                    // If buffer full happened, you should consider increasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(_rxQueue);
                    break;
                    //Event of UART RX break detected
                case UART_BREAK:
                    esp_logi(uart, "uart rx break");
                    break;
                    //Event of UART parity check error
                case UART_PARITY_ERR:
                    esp_logi(uart, "uart parity error");
                    break;
                    //Event of UART frame error
                case UART_FRAME_ERR:
                    esp_logi(uart, "uart frame error");
                    break;
                    //UART_PATTERN_DET
//                case UART_PATTERN_DET: {
//                    uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
//                    int pos = uart_pattern_pop_pos(EX_UART_NUM);
//                    esp_logd(uart, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
//                    if (pos == -1) {
//                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
//                        // record the position. We should set a larger queue size.
//                        // As an example, we directly flush the rx buffer here.
//                        uart_flush_input(EX_UART_NUM);
//                    } else {
//                        uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
//                        uint8_t pat[PATTERN_CHR_NUM + 1];
//                        memset(pat, 0, sizeof(pat));
//                        uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
//                        esp_logd(uart, "read data: %s", dtmp);
//                        esp_logd(uart, "read pat : %s", pat);
//                    }
//                    break;
//                }
                    //Others
                case UART_DATA_BREAK:
                    esp_logi(uart, "uart event: UART_DATA_BREAK");
                    break;
                case UART_EVENT_MAX:
                    esp_logi(uart, "uart event: UART_EVENT_MAX");
                    break;
                default:
                    esp_logi(uart, "uart event: UNKNOWN");
                    break;
            }
        }
    }
    vTaskDelete(nullptr);
}
