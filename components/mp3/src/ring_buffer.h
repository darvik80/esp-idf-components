//
// Created by Ivan Kishchenko on 21/10/24.
//

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <esp_err.h>

enum RingBufferFlags {
    RB_IS_EMPTY = 0x00,
    RB_IS_FULL = 0x01,
};

typedef struct {
    uint8_t *start;
    uint8_t *end;
    volatile uint8_t *write_ptr;
    volatile uint8_t *read_ptr;

    volatile uint8_t flags;
} ring_buffer_t;

typedef ring_buffer_t *ring_buffer_handler_t;

esp_err_t ring_buffer_create(size_t size, ring_buffer_handler_t *ring_buffer);

esp_err_t ring_buffer_write(ring_buffer_handler_t ring_buffer, uint8_t ch);

esp_err_t ring_buffer_read(ring_buffer_handler_t ring_buffer, uint8_t *ch);

esp_err_t ring_buffer_destroy(ring_buffer_handler_t ring_buffer);

#endif //RING_BUFFER_H
