//
// Created by Ivan Kishchenko on 21/10/24.
//

#include "ring_buffer.h"

#include <string.h>

esp_err_t ring_buffer_create(size_t size, ring_buffer_handler_t *ring_buffer) {
    ring_buffer_t *buffer = malloc(sizeof(ring_buffer_t));
    buffer->buf = (uint8_t *) malloc(size);
    buffer->buf_size = size;
    buffer->head = 0;
    buffer->tail = 0;

    *ring_buffer = buffer;
    return ESP_OK;
}

size_t ring_buffer_available(ring_buffer_handler_t ring_buffer) {
    if (ring_buffer->head <= ring_buffer->tail) {
        return ring_buffer->tail - ring_buffer->head;
    }

    return ring_buffer->buf_size - (ring_buffer->head - ring_buffer->tail);
}

esp_err_t ring_buffer_add(ring_buffer_handler_t ring_buffer, uint8_t *buf, size_t size) {
    if (ring_buffer->buf_size < size) {
        return ESP_ERR_NO_MEM;
    }

    if (ring_buffer_available(ring_buffer)) {
        return ESP_ERR_NO_MEM;
    }

    size_t remaining = ring_buffer->buf_size - ring_buffer->tail;
    if (remaining > size) {
        memcpy(ring_buffer->buf + ring_buffer->tail, buf, size);
        ring_buffer->tail += size;
    } else {
        memcpy(ring_buffer->buf + ring_buffer->tail, buf, remaining);
        ring_buffer->tail = size - remaining;
        memcpy(ring_buffer->buf, buf, ring_buffer->tail);
    }

    return ESP_OK;
}

esp_err_t ring_buffer_destroy(ring_buffer_handler_t ring_buffer) {
    free(ring_buffer->buf);
    return ESP_OK;
}
