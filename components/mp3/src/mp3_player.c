//
// Created by Ivan Kishchenko on 18/10/24.
//
#include "mp3_player.h"

#include <esp_check.h>
#include <soc/gpio_num.h>

static const char *TAG = "mp3";

esp_err_t mp3_player_play_next(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->play_next(player);
}

esp_err_t mp3_player_play_prev(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->play_prev(player);
}

esp_err_t mp3_player_play(mp3_player_handle_t player, int16_t idx) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->play(player, idx);
}

esp_err_t mp3_player_destroy(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->destroy(player);
}
