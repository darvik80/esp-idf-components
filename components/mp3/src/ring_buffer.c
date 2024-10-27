//
// Created by Ivan Kishchenko on 21/10/24.
//

#include "ring_buffer.h"

#include <string.h>

esp_err_t ring_buffer_create(size_t size, ring_buffer_handler_t* ring_buffer)
{
    ring_buffer_t* buffer = malloc(sizeof(ring_buffer_t));
    buffer->start = (uint8_t*)malloc(size);
    buffer->end = buffer->start + size;
    buffer->write_ptr = buffer->start;
    buffer->read_ptr = buffer->start;
    buffer->flags = RB_IS_EMPTY;

    *ring_buffer = buffer;
    return ESP_OK;
}

esp_err_t ring_buffer_write(ring_buffer_handler_t ring_buffer, uint8_t ch)
{
    if (ring_buffer == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ((ring_buffer->write_ptr == ring_buffer->read_ptr) && ring_buffer->flags & RB_IS_FULL)
    {
        return ESP_ERR_NO_MEM;
    }

    *(ring_buffer->write_ptr) = ch;

    volatile uint8_t* tmp = ring_buffer->write_ptr + 1;
    if (tmp >= ring_buffer->end) tmp = ring_buffer->start;
    if (tmp == ring_buffer->read_ptr)
    {
        ring_buffer->flags = RB_IS_FULL;
    }
    ring_buffer->write_ptr = tmp;

    return ESP_OK;
}

esp_err_t ring_buffer_read(ring_buffer_handler_t ring_buffer, uint8_t* ch)
{
    if (ring_buffer == NULL) return ESP_ERR_INVALID_ARG;

    if ((ring_buffer->read_ptr == ring_buffer->write_ptr) && ring_buffer->flags == RB_IS_EMPTY)
    {
        return ESP_FAIL;
    }

    *ch = *ring_buffer->read_ptr;

    volatile uint8_t* tmp = ring_buffer->read_ptr + 1;
    if (tmp >= ring_buffer->end) tmp = ring_buffer->start;
    if (tmp == ring_buffer->write_ptr)ring_buffer->flags = RB_IS_EMPTY;
    ring_buffer->read_ptr = tmp;

    return ESP_OK;
}

esp_err_t ring_buffer_destroy(ring_buffer_handler_t ring_buffer)
{
    free(ring_buffer->start);
    free(ring_buffer);

    return ESP_OK;
}
