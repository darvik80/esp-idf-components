//
// Created by Ivan Kishchenko on 22/6/24.
//

#include "UartExchange.h"
#include "core/system/storage/NvsStorage.h"

ESP_EVENT_DEFINE_BASE(UART_EXCHANGE_INTERNAL_EVENT);

void fromJson(const cJSON *json, UartExchangeProperties &props) {
    cJSON *item = json->child;
    while (item) {
        if (item->type == cJSON_Number && !strcmp(item->string, "port")) {
            props.port = static_cast<uart_port_t>(item->valuedouble);
        } else if (item->type == cJSON_Number && !strcmp(item->string, "baud-rate")) {
            props.baudRate = static_cast<uint16_t>(item->valuedouble);
        } else if (item->type == cJSON_Number && !strcmp(item->string, "rx-pin")) {
            props.rxPin = static_cast<gpio_num_t>(item->valuedouble);
        } else if (item->type == cJSON_Number && !strcmp(item->string, "tx-pin")) {
            props.txPin = static_cast<gpio_num_t>(item->valuedouble);
        }

        item = item->next;
    }
}

UartExchange::UartExchange(Registry &registry) : TService(registry) {
    registry.getPropsLoader().addReader("uart", defaultPropertiesReader<UartExchangeProperties>);
    //registry.getPropsLoader().addConsumer(this);
}

void UartExchange::setup() {
    ESP_ERROR_CHECK(esp_event_handler_register(UART_EXCHANGE_INTERNAL_EVENT, ESP_EVENT_ANY_ID, eventHandler, this));

    uart_config_t uart_config = {
            .baud_rate = _props.baudRate,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(_props.port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(_props.port, _props.txPin, _props.rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(_props.port, 1024 * 2, 1024 * 2, 256, &_rxQueue, 0));

    xTaskCreate(rxTask, "uart-rx-task", 8192, this, tskIDLE_PRIORITY + 1, nullptr);

    _txQueue = xQueueCreate(32, sizeof(UartMessage));
    xTaskCreate(txTask, "uart_tx_task", 4096, this, tskIDLE_PRIORITY, nullptr);

    _mode = (Mode) getRegistry().getService<NvsStorage>()->getInt8("uart-mode");

    _channel = std::make_unique<ZeroMQChannel>(_mode);
    _channel->onConnect();

    core_logi(uart, "uart-exchange started: {}, port: {}, baud-rate: {}", _mode == Master ? "master" : "slave", (int)_props.port, _props.baudRate);
}

void UartExchange::eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == UART_EXCHANGE_INTERNAL_EVENT) {
        switch (event_id) {
            case Uart_Exchange_Rx: {
                auto *msg = (UartMessage *) event_data;
                _channel->onRecv(msg->data, msg->size);
                freeUartMessage(*msg);
            }
                break;
            default:
                break;
        }
    }
}

void UartExchange::apply(const UartExchangeProperties &props) {
    _props = props;
}

[[noreturn]] void UartExchange::rxTask() {
    esp_logi(uart, "rx-task started");
    uart_event_t event;
    std::array<char, 1024> buf{};
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(_rxQueue, (void *) &event, (TickType_t) portMAX_DELAY)) {
            switch (event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA: {
                    core_logi(uart, "recv-data, port: {}, size: {}", (int)_props.port, event.size);
                    auto len = uart_read_bytes(_props.port, buf.data(), event.size, portMAX_DELAY);
                    auto uartMsg = makeUartMessage(buf.data(), len);
                    esp_event_post(UART_EXCHANGE_INTERNAL_EVENT, Uart_Exchange_Rx, &uartMsg, sizeof(uartMsg), portMAX_DELAY);
                    break;
                }

                    break;
                    //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    esp_logw(uart, "UART_FIFO_OVF");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(_props.port);
                    xQueueReset(_rxQueue);
                    break;
                    //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    esp_logw(uart, "UART_BUFFER_FULL");
                    // If buffer full happened, you should consider increasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(_props.port);
                    xQueueReset(_rxQueue);
                    break;
                    //Event of UART RX break detected
                case UART_BREAK:
                    esp_logd(uart, "UART_BREAK");
                    break;
                    //Event of UART parity check error
                case UART_PARITY_ERR:
                    esp_logd(uart, "UART_PARITY_ERR");
                    break;
                    //Event of UART frame error
                case UART_FRAME_ERR:
                    esp_logd(uart, "UART_FRAME_ERR");
                    break;
                    //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    esp_logd(uart, "UART_PATTERN_DET");
                    break;
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
                default:
                    esp_logd(uart, "UART_EVENT: %d", event.type);
                    break;
            }
        }
    }
    vTaskDelete(nullptr);
    esp_logi(uart, "rx-task stopped");
}

void UartExchange::txTask() {
    esp_logi(uart, "tx-task started");
    while (true) {
        UartMessage msg;
        if (xQueueReceive(_txQueue, &msg, pdMS_TO_TICKS(10000))) {
            uart_write_bytes(_props.port, msg.data, msg.size);
            freeUartMessage(msg);
        } else if (_mode == Master) {
            esp_logi(uart, "port: %d, send: ping", _props.port);
            uart_write_bytes(_props.port, "ping", 4);
        } else {
            //esp_logi(uart, "timeout");
        }
    }
    esp_logi(uart, "tx-task stopped");
    vTaskDelete(nullptr);
}

bool UartExchange::post(const void *data, size_t size, TickType_t delay) {
    auto msg = makeUartMessage(data, size);
    return pdTRUE == xQueueSend(_txQueue, &msg, delay);
}
