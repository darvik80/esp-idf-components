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

    esp_err_t (*play)(struct mp3_player_t *self, uint16_t idx);

    esp_err_t (*volume_increase)(struct mp3_player_t *self);

    esp_err_t (*volume_decrease)(struct mp3_player_t *self);

    esp_err_t (*volume)(struct mp3_player_t *self, uint8_t idx);

    esp_err_t (*eq)(struct mp3_player_t *self, uint8_t idx);

    esp_err_t (*play_mode)(struct mp3_player_t *self, uint8_t idx);

    esp_err_t (*stand_by)(struct mp3_player_t *self);

    esp_err_t (*normal)(struct mp3_player_t *self);

    esp_err_t (*reset)(struct mp3_player_t *self);

    esp_err_t (*playback)(struct mp3_player_t *self);

    esp_err_t (*pause)(struct mp3_player_t *self);

    esp_err_t (*destroy)(struct mp3_player_t *self);
};

esp_err_t mp3_player_play_next(mp3_player_handle_t player);

esp_err_t mp3_player_play_prev(mp3_player_handle_t player);

esp_err_t mp3_player_play(mp3_player_handle_t player, uint16_t idx);

esp_err_t mp3_player_volume_increase(mp3_player_handle_t player);

esp_err_t mp3_player_volume_decrease(mp3_player_handle_t player);

esp_err_t mp3_player_volume(mp3_player_handle_t player, uint8_t idx);

esp_err_t mp3_player_eq(mp3_player_handle_t player, uint8_t idx);

esp_err_t mp3_player_play_mode(mp3_player_handle_t player, uint8_t idx);

esp_err_t mp3_player_stand_by(mp3_player_handle_t player);

esp_err_t mp3_player_normal(mp3_player_handle_t player);

esp_err_t mp3_player_reset(mp3_player_handle_t player);

esp_err_t mp3_player_playback(mp3_player_handle_t player);

esp_err_t mp3_player_pause(mp3_player_handle_t player);

esp_err_t mp3_player_destroy(mp3_player_handle_t player);

#endif //MP3_PLAYER_H
