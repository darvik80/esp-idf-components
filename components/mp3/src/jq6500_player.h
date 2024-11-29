//
// Created by Ivan Kishchenko on 18/10/24.
//

#pragma once

#include <hal/uart_types.h>
#include <soc/gpio_num.h>

#include "mp3_player.h"

typedef struct {
    uart_port_t port;
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
    int baud_rate;

    gpio_num_t busy_pin;
    int rx_buffer_size;
    int tx_buffer_size;
} jq6500_player_config_t;

esp_err_t create_jq6500_player(jq6500_player_config_t *config, mp3_player_handle_t *handle);

