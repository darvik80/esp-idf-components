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

esp_err_t mp3_player_play(mp3_player_handle_t player, uint16_t idx) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->play(player, idx);
}

esp_err_t mp3_player_volume_increase(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->volume_increase(player);
}

esp_err_t mp3_player_volume_decrease(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->volume_decrease(player);
}

esp_err_t mp3_player_volume(mp3_player_handle_t player, uint8_t idx) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->volume(player, idx);
}

esp_err_t mp3_player_eq(mp3_player_handle_t player, uint8_t idx) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->eq(player, idx);
}

esp_err_t mp3_player_play_mode(mp3_player_handle_t player, uint8_t idx) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->play_mode(player, idx);
}

esp_err_t mp3_player_stand_by(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->stand_by(player);
}

esp_err_t mp3_player_normal(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->normal(player);
}

esp_err_t mp3_player_reset(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->reset(player);
}

esp_err_t mp3_player_playback(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->playback(player);
}

esp_err_t mp3_player_pause(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->pause(player);
}

esp_err_t mp3_player_destroy(mp3_player_handle_t player) {
    ESP_RETURN_ON_FALSE(player, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

    return player->destroy(player);
}
