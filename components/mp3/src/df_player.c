//
// Created by Ivan Kishchenko on 18/10/24.
//

#include "df_player.h"

#include <cc.h>
#include <esp_check.h>
#include <string.h>
#include <driver/uart.h>
#include <sys/param.h>

static const char *TAG = "df-mp3";

typedef struct {
    struct mp3_player_t base;
    uart_port_t port;
    gpio_num_t busy_pin;
    QueueHandle_t uart_queue;
} df_player_t;

const uint8_t beg = 0x7e, end = 0xef, len = 0x06, ver = 0xff;

typedef struct {
    uint8_t magicBegin{beg};
    uint8_t version{ver};
    uint8_t length{len};
    uint8_t code{};
    uint8_t feedback{};

    union {
        struct {
            uint8_t arg1{};
            uint8_t arg2{};
        };

        uint16_t arg;
    };

    uint16_t checksum{};
    uint8_t magicEnd{end};
} df_message;

esp_err_t df_play_next(struct mp3_player_t *self) {
    df_player_t *player = __containerof(self, df_player_t, base);

    return ESP_OK;
}

esp_err_t df_play_prev(struct mp3_player_t *self) {
    return ESP_OK;
}

esp_err_t df_play(struct mp3_player_t *self, int16_t idx) {
    return ESP_OK;
}

uint16_t checksum(df_message *msg) {
    uint8_t* buf = msg;
    uint16_t sum = 0;
    for (size_t idx = 1; idx <= 6; idx++) {
        sum -= buf[idx];
    }

    return htons(sum);
}

size_t detect(uint8_t *buf, size_t size) {
    size_t offset = 0;
    while (size - offset > sizeof(df_message)) {
        df_message *msg = (df_message*)(buf + offset);
        if (msg->magicBegin == beg && msg->magicEnd == end && msg->length == len) {
            if (msg->checksum == checksum(msg)) {
                //onMessage(*msg);
            }
            offset += sizeof(df_message);
        } else {
            offset++;
        }
    }

    return offset;
}

void df_player_rx_task(void *args) {
    size_t offset = 0, buffer_size = 256, size = 0;
    uint8_t* rxBuf = malloc(buffer_size);

    df_player_t *self = args;

    ESP_LOGI(TAG, "rxTask running");
    uart_flush_input(self->port);
    xQueueReset(self->uart_queue);

    while (true) {
        // send data
        uart_event_t event{};
        if (xQueueReceive(self->uart_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: {
                    size_t cached{0};
                    uart_get_buffered_data_len(self->port, &cached);
                    while (cached) {
                        size_t blockSize = MIN(cached, buffer_size-size);
                        uart_read_bytes(self->port, rxBuf+offset, blockSize, portMAX_DELAY);
                        offset = detect(rxBuf, cached);
                        if (offset) {
                            memmove(rxBuf, rxBuf+offset, 256-offset);
                        }

                        cached -= blockSize;
                    }
                }
                break;
                default: {
                    ESP_LOGD(TAG, "uart[%d] event: %d", self->port, event.type);
                    uart_flush_input(self->port);
                    xQueueReset(self->uart_queue);
                }
                break;
            }
        }
    }

    free(rxBuf);
}

esp_err_t create_df_player(df_player_config_t *config, mp3_player_handle_t *handle) {
    df_player_t *player = malloc(sizeof(df_player_t));
    uart_config_t uart_config = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };

    // Configure UART parameters
    ESP_RETURN_ON_ERROR(uart_param_config(config->port, &uart_config), TAG, "failed to configure UART");
    ESP_RETURN_ON_ERROR(uart_set_pin(
                            config->port, config->tx_pin, config->rx_pin, -1, -1),
                        TAG, "failed to set TX/RX pins to UART"
    );

    QueueHandle_t uart_queue{};
    ESP_RETURN_ON_ERROR(uart_driver_install(
                            config->port, config->rx_buffer_size,
                            config->rx_buffer_size, 1, &uart_queue, 0),
                        TAG, "failed to install UART"
    );

    player->port = config->port;
    player->busy_pin = config->busy_pin;
    player->uart_queue = uart_queue;
    player->base.play_next = df_play_next;
    player->base.play_prev = df_play_prev;
    player->base.play = df_play;
    *handle = &player->base;

    xTaskCreate(df_player_rx_task, "df_player_rx", 2048, player, 2, NULL);

    return ESP_OK;
}
