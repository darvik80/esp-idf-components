//
// Created by Ivan Kishchenko on 8/3/24.
//

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

template<typename T>
class FreeRTOSQueue {
private:
    QueueHandle_t _handle;

public:
    FreeRTOSQueue(const FreeRTOSQueue &) = delete;

    FreeRTOSQueue &operator=(const FreeRTOSQueue &) = delete;

    explicit FreeRTOSQueue(uint32_t queueLength) {
        _handle = xQueueCreate(queueLength, sizeof(T));
    }

    FreeRTOSQueue(FreeRTOSQueue &&other) {
        _handle = other._handle;
        other._handle = nullptr;
    }

    FreeRTOSQueue &operator=(FreeRTOSQueue &&other) {
        if (this != &other) {
            _handle = other._handle;
            other._handle = nullptr;
        }
        return *this;
    }

    bool enqueue(const T &item, TickType_t ticksToWait = 0) {
        return xQueueSend(_handle, &item, ticksToWait) == pdPASS;
    }

    bool enqueueFromISR(const T &item, BaseType_t *higherPriorityTaskWoken) {
        return xQueueSendFromISR(_handle, &item, higherPriorityTaskWoken) == pdPASS;
    }

    bool dequeue(T &item, TickType_t ticksToWait = portMAX_DELAY) {
        return xQueueReceive(_handle, &item, ticksToWait) == pdPASS;
    }

    bool dequeueFromISR(T &item, BaseType_t *higherPriorityTaskWoken) {
        return xQueueReceiveFromISR(_handle, &item, higherPriorityTaskWoken) == pdPASS;
    }

    bool peek(T &item, TickType_t ticksToWait = 0) {
        return xQueuePeek(_handle, &item, ticksToWait) == pdPASS;
    }

    [[nodiscard]] bool isFull() const {
        return uxQueueSpacesAvailable(_handle) == 0;
    }

    [[nodiscard]] bool isEmpty() const {
        return uxQueueMessagesWaiting(_handle) == 0;
    }

    bool overwrite(const T &item) {
        return xQueueOverwrite(_handle, &item) == pdPASS;
    }

    bool overwriteFromISR(const T &item, BaseType_t *higherPriorityTaskWoken) {
        return xQueueOverwriteFromISR(_handle, &item, higherPriorityTaskWoken) == pdPASS;
    }

    [[nodiscard]] UBaseType_t messagesWaiting() const {
        return uxQueueMessagesWaiting(_handle);
    }

    [[nodiscard]] UBaseType_t spacesAvailable() const {
        return uxQueueSpacesAvailable(_handle);
    }

    ~FreeRTOSQueue() {
        if (_handle != nullptr) {
            vQueueDelete(_handle);
        }
    }
};