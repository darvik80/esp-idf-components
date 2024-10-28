//
// Created by Ivan Kishchenko on 18/10/24.
//

#include "df_player.h"

#include <cc.h>
#include <esp_check.h>
#include <string.h>
#include <driver/gpio.h>
#include <driver/uart.h>

static const char *TAG = "df-mp3";

enum DFPlayerCode {
    DF_PlayNext = 0x01,
    DF_PlayPrev = 0x02,
    DF_Play = 0x03, // 1 - 2999
    DF_VolumeIncrease = 0x04,
    DF_VolumeDecrease = 0x05,
    DF_VolumeSet = 0x06, // 1 - 30
    DF_EQ = 0x07, // Normal/Pop/Rock/Jazz/Classic/Base
    DF_PlayMode = 0x08, // Repeat/Folder Repeat/Single Repeat/Random
    DF_DataSource = 0x09, // U/TF/AUX/SLEEP/FLASH
    DF_StandBy = 0x0A,
    DF_Normal = 0x0B,
    DF_Reset = 0x0C,
    DF_Playback = 0x0D,
    DF_Pause = 0x0E,
    DF_SetFolder = 0x0F, // 1 - 10
    DF_VolumeAdjustSet = 0x10,
    DF_RepeatPlay = 0x11,
    DF_STAY1 = 0x3C,
    DF_STAY2 = 0x3D,
    DF_STAY3 = 0x3E,
    DF_Init = 0x3F,
    DF_ERROR = 0x40,
    DF_OK = 0x41,
    DF_GET_CUR_STATUS = 0x42,
    DF_GET_CUR_VOLUME = 0x43,
    DF_GET_CUR_EQ = 0x44,
    DF_GET_CUR_PLAYBACK_MODE = 0x45,
    DF_GET_VERSION = 0x46,
    DF_GET_TF_FILES_COUNT = 0x47,
    DF_GET_FLASH_FILES_COUNT = 0x49,
    DF_GET_TF_CUR_FILE_ID = 0x4B,
    DF_GET_U_CUR_FILE_ID = 0x4C,
    DF_GET_FLASH_CUR_FILE_ID = 0x4D,
};


typedef struct {
    struct mp3_player_t base;
    uart_port_t port;
    gpio_num_t busy_pin;
    QueueHandle_t uart_queue;
    QueueHandle_t rx_queue;
    TaskHandle_t rx_task;
} df_player_t;

const uint8_t beg = 0x7e, end = 0xef, len = 0x06, ver = 0xff;

#pragma pack(push, 1)

typedef struct {
    uint8_t magicBegin;
    uint8_t version;
    uint8_t length;
    uint8_t code;
    uint8_t feedback;

    union {
        struct {
            uint8_t arg1;
            uint8_t arg2;
        };

        uint16_t arg;
    };

    uint16_t checksum;
    uint8_t magicEnd;
} df_message;

#pragma pack(pop)

uint16_t checksum(df_message *msg) {
    const uint8_t *buf = (uint8_t *) msg;
    uint16_t sum = 0;
    for (size_t idx = 1; idx <= 6; idx++) {
        sum -= buf[idx];
    }

    return htons(sum);
}

esp_err_t df_exec(struct mp3_player_t *self, uint8_t cmd, uint16_t arg) {
    df_player_t *player = __containerof(self, df_player_t, base);

    ESP_LOGI(TAG, "Exec CMD REQ: %.02x, %d", cmd, arg);
    df_message msg = {
        .magicBegin = beg,
        .version = ver,
        .length = len,
        .code = cmd,
        .feedback = 0x01,
        .arg = htons(arg),
        .magicEnd = end,
    };
    msg.checksum = checksum(&msg);
    uart_write_bytes(player->port, &msg, sizeof(df_message));
    xQueueReceive(player->rx_queue, &msg, portMAX_DELAY);
    ESP_LOGI(TAG, "Exec CMD ACK: %.02x/%.02x", cmd, msg.code);

    return msg.code == DF_OK || msg.code == DF_STAY2 ? ESP_OK : ESP_FAIL;
}


esp_err_t df_play_next(struct mp3_player_t *self) {
    return df_exec(self, DF_PlayNext, 0);
}

esp_err_t df_play_prev(struct mp3_player_t *self) {
    return df_exec(self, DF_PlayPrev, 0);
}

esp_err_t df_play_idx(struct mp3_player_t *self, uint16_t idx) {
    return df_exec(self, DF_Play, idx);
}

esp_err_t df_volume_increase(struct mp3_player_t *self) {
    return df_exec(self, DF_VolumeIncrease, 0);
}

esp_err_t df_volume_decrease(struct mp3_player_t *self) {
    return df_exec(self, DF_VolumeDecrease, 0);
}

esp_err_t df_volume(struct mp3_player_t *self, uint8_t idx) {
    return df_exec(self, DF_VolumeSet, idx);
}

esp_err_t df_eq(mp3_player_handle_t player, uint8_t idx) {
    return df_exec(player, DF_EQ, idx);
}

esp_err_t df_play_mode(mp3_player_handle_t player, uint8_t idx) {
    return df_exec(player, DF_PlayMode, idx);
}

esp_err_t df_stand_by(mp3_player_handle_t player) {
    return df_exec(player, DF_StandBy, 0);
}

esp_err_t df_normal(mp3_player_handle_t player) {
    return df_exec(player, DF_Normal, 0);
}

esp_err_t df_reset(mp3_player_handle_t player) {
    return df_exec(player, DF_Reset, 0);
}

esp_err_t df_playback(mp3_player_handle_t player) {
    return df_exec(player, DF_Playback, 0);
}

esp_err_t df_pause(mp3_player_handle_t player) {
    return df_exec(player, DF_Pause, 0);
}

esp_err_t df_destroy(mp3_player_handle_t player) {
    const df_player_t *self = __containerof(player, df_player_t, base);
    vTaskDelete(self->rx_task);
    vQueueDelete(self->rx_queue);
    uart_driver_delete(self->port);
    free(player);
    return ESP_OK;
}

[[noreturn]] void df_player_rx_task(void *args) {
    const df_player_t *self = args;
    uint8_t msg[sizeof(df_message)];
    uint8_t fragmented = 0;

    ESP_LOGI(TAG, "rxTask running");
    uart_flush_input(self->port);
    xQueueReset(self->uart_queue);

    while (true) {
        // send data
        uart_event_t event;
        if (xQueueReceive(self->uart_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA: {
                    ESP_LOGI(TAG, "recv data: %d", event.size);
                    size_t size;
                    uart_get_buffered_data_len(self->port, &size);
                    if (size < sizeof(df_message) && !fragmented) {
                        break;
                    }

                    while (size) {
                        if (!fragmented) {
                            uart_read_bytes(self->port, &msg, sizeof(df_message), portMAX_DELAY);
                            size -= sizeof(df_message);
                        } else {
                            memmove(&msg[0], &msg[1], sizeof(df_message) - 1);
                            uart_read_bytes(self->port, &msg[sizeof(df_message) - 1], sizeof(uint8_t), portMAX_DELAY);
                            size--;
                        }
                        df_message *ptr = (df_message *) msg;

                        ESP_LOG_BUFFER_HEX(TAG, msg, sizeof(msg));

                        if (ptr->magicBegin == beg && ptr->magicEnd == end && ptr->length == len && ptr->checksum ==
                            checksum(ptr)) {
                            xQueueSendToBack(self->rx_queue, ptr, portMAX_DELAY);
                            fragmented = 0;
                        } else {
                            fragmented = 1;
                        }
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
}

esp_err_t create_df_player(df_player_config_t *config, mp3_player_handle_t *handle) {
    df_player_t *player = malloc(sizeof(df_player_t));
    uart_config_t uart_config = {
        .baud_rate = config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    // Configure UART parameters
    ESP_RETURN_ON_ERROR(uart_param_config(config->port, &uart_config), TAG, "failed to configure UART");
    ESP_RETURN_ON_ERROR(uart_set_pin(
                            config->port, config->tx_pin, config->rx_pin, -1, -1),
                        TAG, "failed to set TX/RX pins to UART"
    );

    QueueHandle_t uart_queue = NULL;
    ESP_RETURN_ON_ERROR(
        uart_driver_install(
            config->port, config->rx_buffer_size,
            config->tx_buffer_size, 1, &uart_queue, 0
        ),
        TAG, "failed to install UART"
    );

    player->port = config->port;
    player->busy_pin = config->busy_pin;
    player->uart_queue = uart_queue;
    player->base.play_next = df_play_next;
    player->base.play_prev = df_play_prev;
    player->base.play = df_play_idx;
    player->base.volume_increase = df_volume_increase;
    player->base.volume_increase = df_volume_decrease;
    player->base.volume = df_volume;
    player->base.eq = df_eq;
    player->base.play_mode = df_play_mode;
    player->base.stand_by = df_stand_by;
    player->base.normal = df_normal;
    player->base.reset = df_reset;
    player->base.playback = df_playback;
    player->base.pause = df_pause;
    player->base.destroy = df_destroy;
    player->rx_queue = xQueueCreate(1, sizeof(df_message));
    *handle = &player->base;

    gpio_config_t io_conf_busy = {
        .pin_bit_mask = BIT64(config->busy_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_busy);

    xTaskCreate(df_player_rx_task, "df_player_rx", 3072, player, 2, &player->rx_task);

    return ESP_OK;
}
