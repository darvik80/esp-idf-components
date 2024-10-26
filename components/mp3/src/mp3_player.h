//
// Created by Ivan Kishchenko on 18/10/24.
//

#ifndef MP3_PLAYER_H
#define MP3_PLAYER_H

#include <esp_err.h>

typedef struct mp3_player_t *mp3_player_handle_t;

struct mp3_player_t {
    esp_err_t (*play_next)(struct mp3_player_t *self);

    esp_err_t (*play_prev)(struct mp3_player_t *self);

    esp_err_t (*play)(struct mp3_player_t *self, int16_t idx);

    esp_err_t (*destroy)(struct mp3_player_t *self);
};

esp_err_t mp3_player_play_next(mp3_player_handle_t player);

esp_err_t mp3_player_play_prev(mp3_player_handle_t player);

esp_err_t mp3_player_play(mp3_player_handle_t player, int16_t idx);

esp_err_t mp3_player_destroy(mp3_player_handle_t player);

#endif //MP3_PLAYER_H
