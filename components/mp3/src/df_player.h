//
// Created by Ivan Kishchenko on 18/10/24.
//

#ifndef DF_PLAYER_H
#define DF_PLAYER_H

#include <hal/uart_types.h>
#include <soc/gpio_num.h>

#include "mp3_player.h"

typedef struct {
    uart_port_t port;
    gpio_num_t tx_pin{-1};
    gpio_num_t rx_pin{-1};
    int baud_rate{9600};

    gpio_num_t busy_pin{-1};
    int rx_buffer_size{64};
    int tx_buffer_size{64};
} df_player_config_t;

esp_err_t create_df_player(df_player_config_t* config, mp3_player_handle_t* handle);

#endif //DF_PLAYER_H
