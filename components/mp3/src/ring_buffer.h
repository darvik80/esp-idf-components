//
// Created by Ivan Kishchenko on 21/10/24.
//

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <esp_err.h>

typedef struct {
    uint8_t *buf;
    size_t buf_size;
    size_t head;
    size_t tail;
} ring_buffer_t;

typedef ring_buffer_t* ring_buffer_handler_t;

esp_err_t ring_buffer_create(size_t size, ring_buffer_handler_t* ring_buffer);

esp_err_t ring_buffer_add(ring_buffer_handler_t ring_buffer, uint8_t *buf, size_t size);

esp_err_t ring_buffer_destroy(ring_buffer_handler_t ring_buffer);

#endif //RING_BUFFER_H
