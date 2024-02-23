//
// Created by Ivan Kishchenko on 17/02/2024.
//

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Task.h"

template<typename T, size_t queueSize, std::enable_if_t<std::is_trivially_copyable<T>::value, bool> = true>
class FreeRTOSMessageBus {
    QueueHandle_t _handler{nullptr};
    FreeRTOSTask _task;
    std::function<void(const T &msg)> _callback;
private:
public:
    explicit FreeRTOSMessageBus(std::function<void(const T &msg)> callback, const TaskOptions &options)
            : _callback(callback) {
        _handler = xQueueCreate(queueSize, sizeof(T));

        _task = FreeRTOSTask::submit(
                [this]() {
                    T msg;
                    while (xQueueReceive(_handler, &msg, portMAX_DELAY)) {
                        _callback(msg);
                    }
                },
                options.name.c_str(),
                options.stackSize,
                options.priority
        );
    }

    bool post(const T &msg, TickType_t ticksWait) {
        return pdPASS == xQueueSend(_handler, &msg, ticksWait);
    }

    /*
     * queue overwrite works only with queueSize = 1
     */
    template<std::enable_if_t<(queueSize == 1), bool> = true>
    bool overwrite(const T &msg) {
        return pdPASS == xQueueOverwrite(_handler, &msg);
    }

    ~FreeRTOSMessageBus() {
        vQueueDelete(_handler);
    }
};
