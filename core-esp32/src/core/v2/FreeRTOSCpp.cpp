//
// Created by Ivan Kishchenko on 10/02/2024.
//
#include "FreeRTOSCpp.h"

namespace freertos {

    Timer::Timer(Timer &&other) {
        _timer = other._timer;
        other._timer = nullptr;
    }

    Timer &Timer::operator=(Timer &&other) {
        if (this != &other) {
            _timer = other._timer;
            other._timer = nullptr;
        }
        return *this;
    }

    bool Timer::attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) {
        shutdown();
        _callback = callback;
        _timer = xTimerCreate(
                "timer",
                pdMS_TO_TICKS(milliseconds),
                repeat,
                this,
                Timer::onCallback
        );

        return pdPASS == xTimerStart(_timer, 0);
    }

    void Timer::shutdown() {
        if (_timer) {
            xTimerStop(_timer, 0);
            xTimerDelete(_timer, 0);
            _timer = nullptr;
        }
    }

    Timer::~Timer() {
        shutdown();
    }
}

namespace experimental {
#if defined (CONFIG_IDF_TARGET)
    ESP_EVENT_DEFINE_BASE(MAIN_EVENT);
#endif

#ifdef CONFIG_BUS_FREE_RTOS_ENABLED

    IMessageBus &getDefaultMessageBus() {
        static FreeRTOSMessageBus bus{
                "main-bus",
                {
                        .stackSize = CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE,
                        .priority = tskIDLE_PRIORITY,
                        .queueSize = CONFIG_BUS_FREE_RTOS_QUEUE_SIZE,
                        .useSystemQueue = false,
                }
        };
#else
#if defined (CONFIG_IDF_TARGET)
#ifdef CONFIG_BUS_ESP_EVENT_LOOP_ENABLED
        static EspIdfEventBus bus{
            "main-bus",
            .stackSize = CONFIG_ESP_EVENT_TASK_STACK_SIZE,
            .priority = tskIDLE_PRIORITY,
            .queueSize = CONFIG_ESP_EVENT_QUEUE_SIZE,
            .useSystemQueue = false,
        };
#endif
#endif
#endif

        return bus;
    }
}