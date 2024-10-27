//
// Created by Ivan Kishchenko on 18/10/24.
//

#include "jq6500_player.h"

#include <cc.h>
#include <esp_check.h>
#include <string.h>
#include <driver/uart.h>

static const char* TAG = "jq6500-mp3";

enum JQ6500PlayerCode
{
    JQ6500_PlayNext = 0x01,
    JQ6500_PlayPrev = 0x02,
    JQ6500_Play = 0x03, // 1 - 2999
    JQ6500_VolumeIncrease = 0x04,
    JQ6500_VolumeDecrease = 0x05,
    JQ6500_VolumeSet = 0x06, // 1 - 30
    JQ6500_EQ = 0x07, // Normal/Pop/Rock/Jazz/Classic/Base
    JQ6500_PlayMode = 0x08, // Repeat/Folder Repeat/Single Repeat/Random
    JQ6500_DataSource = 0x09, // U/TF/AUX/SLEEP/FLASH
    JQ6500_StandBy = 0x0A,
    JQ6500_Normal = 0x0B,
    JQ6500_Reset = 0x0C,
    JQ6500_Playback = 0x0D,
    JQ6500_Pause = 0x0E,
    JQ6500_SetFolder = 0x0F, // 1 - 10
    JQ6500_VolumeAdjustSet = 0x10,
    JQ6500_RepeatPlay = 0x11,

    JQ6500_GET_CUR_STATUS = 0x42,
    JQ6500_GET_CUR_VOLUME = 0x43,
    JQ6500_GET_CUR_EQ = 0x44,
    JQ6500_GET_CUR_PLAYBACK_MODE = 0x45,
};


typedef struct
{
    struct mp3_player_t base;
    uart_port_t port;
    gpio_num_t busy_pin;
    QueueHandle_t uart_queue;
    QueueHandle_t rx_queue;
    TaskHandle_t rx_task;
} jq6500_player_t;

const uint8_t jqbeg = 0x7e, jqend = 0xef, jqlen = 0x04;

#pragma pack(push, 1)

typedef struct
{
    uint8_t magicBegin;
    uint8_t length;
    uint8_t code;

    union
    {
        struct
        {
            uint8_t arg1;
            uint8_t arg2;
        };

        uint16_t arg;
    };

    uint8_t magicEnd;
} jq6500_message;

#pragma pack(pop)

esp_err_t exec(struct mp3_player_t* self, uint8_t cmd, uint16_t arg)
{
    jq6500_player_t* player = __containerof(self, jq6500_player_t, base);

    ESP_LOGI(TAG, "Exec CMD REQ: %.02x, %d", cmd, arg);
    jq6500_message msg = {
        .magicBegin = jqbeg,
        .length = jqlen,
        .code = cmd,
        .arg = htons(arg),
        .magicEnd = jqend,
    };
    uart_write_bytes(player->port, &msg, sizeof(jq6500_message));

    return ESP_OK;
}


esp_err_t jq6500_play_next(struct mp3_player_t* self)
{
    return exec(self, JQ6500_PlayNext, 0);
}

esp_err_t jq6500_play_prev(struct mp3_player_t* self)
{
    return exec(self, JQ6500_PlayPrev, 0);
}

esp_err_t jq6500_play_idx(struct mp3_player_t* self, int16_t idx)
{
    return exec(self, JQ6500_Play, idx);
}

esp_err_t jq6500_volume_increase(struct mp3_player_t* self)
{
    return exec(self, JQ6500_VolumeIncrease, 0);
}

esp_err_t jq6500_volume_decrease(struct mp3_player_t* self)
{
    return exec(self, JQ6500_VolumeDecrease, 0);
}

esp_err_t jq6500_volume(struct mp3_player_t* self, int16_t idx)
{
    return exec(self, JQ6500_VolumeSet, idx);
}

esp_err_t jq6500_eq(mp3_player_handle_t player, int16_t idx)
{
    return exec(player, JQ6500_EQ, idx);
}

esp_err_t jq6500_play_mode(mp3_player_handle_t player, int16_t idx)
{
    return exec(player, JQ6500_PlayMode, idx);
}

esp_err_t jq6500_stand_by(mp3_player_handle_t player)
{
    return exec(player, JQ6500_StandBy, 0);
}

esp_err_t jq6500_normal(mp3_player_handle_t player)
{
    return exec(player, JQ6500_Normal, 0);
}

esp_err_t jq6500_reset(mp3_player_handle_t player)
{
    return exec(player, JQ6500_Reset, 0);
}

esp_err_t jq6500_playback(mp3_player_handle_t player)
{
    return exec(player, JQ6500_Playback, 0);
}

esp_err_t jq6500_pause(mp3_player_handle_t player)
{
    return exec(player, JQ6500_Pause, 0);
}

esp_err_t jq6500_destroy(mp3_player_handle_t player)
{
    const jq6500_player_t* self = __containerof(player, jq6500_player_t, base);
    vTaskDelete(self->rx_task);
    vQueueDelete(self->rx_queue);
    uart_driver_delete(self->port);
    free(player);
    return ESP_OK;
}

[[noreturn]] void jq6500_player_rx_task(void* args)
{
    const jq6500_player_t* self = args;

    ESP_LOGI(TAG, "rxTask running");
    uart_flush_input(self->port);
    xQueueReset(self->uart_queue);

    while (true)
    {
        // send data
        uart_event_t event;
        if (xQueueReceive(self->uart_queue, &event, portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                {
                    ESP_LOGI(TAG, "recv data: %d", event.size);
                    uint8_t* buf = malloc(event.size + 1);
                    uart_read_bytes(self->port, buf, event.size, portMAX_DELAY);
                    buf[event.size] = 0;
                    ESP_LOGI(TAG, "recv data: %d:%s", event.size, buf);
                }
                break;
            default:
                {
                    ESP_LOGI(TAG, "uart[%d] event: %d", self->port, event.type);
                    uart_flush_input(self->port);
                    xQueueReset(self->uart_queue);
                }
                break;
            }
        }
    }
}

esp_err_t create_jq6500_player(jq6500_player_config_t* config, mp3_player_handle_t* handle)
{
    jq6500_player_t* player = malloc(sizeof(jq6500_player_t));
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
    player->base.play_next = jq6500_play_next;
    player->base.play_prev = jq6500_play_prev;
    player->base.play = jq6500_play_idx;
    player->base.volume_increase = jq6500_volume_increase;
    player->base.volume_increase = jq6500_volume_decrease;
    player->base.volume = jq6500_volume;
    player->base.eq = jq6500_eq;
    player->base.play_mode = jq6500_play_mode;
    player->base.stand_by = jq6500_stand_by;
    player->base.normal = jq6500_normal;
    player->base.reset = jq6500_reset;
    player->base.playback = jq6500_playback;
    player->base.pause = jq6500_pause;
    player->base.destroy = jq6500_destroy;
    player->rx_queue = xQueueCreate(1, sizeof(jq6500_message));
    *handle = &player->base;

    xTaskCreate(jq6500_player_rx_task, "jq6500_player_rx", 2048, player, 2, &player->rx_task);

    return ESP_OK;
}
