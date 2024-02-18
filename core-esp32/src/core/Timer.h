//
// Created by Ivan Kishchenko on 28/07/2023.
//

#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "EventBus.h"
#include "core/system/SystemEvent.h"

class Timer {
public:
    typedef std::shared_ptr<Timer> Ptr;

    virtual void attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) = 0;

    template<uint8_t tid>
    void fire(uint32_t milliseconds, bool repeat) {
        attach(milliseconds, repeat, []() {
            TimerEvent<tid> event;
            getDefaultEventBus().post(event);
        });
    }

    virtual ~Timer() = default;
};

#ifndef CONFIG_IDF_TARGET_LINUX

extern "C" {
#include "esp_timer.h"
}

class EspTimer : public Timer {
    std::string _name;
    std::function<void()> _callback;

    esp_timer_handle_t _timer{};
private:
    static void onCallback(void *arg) {
        auto *timer = static_cast<EspTimer *>(arg);
        timer->doCallback();
    }

    void doCallback() {
        _callback();
    }

public:
    EspTimer();

    explicit EspTimer(std::string_view name);

    void attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) override;

    void detach();

    ~EspTimer() override {
        detach();
    }
};

#endif

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

class FreeRTOSTimer : public Timer {
    TimerHandle_t _timer{};

    std::function<void()> _callback;
private:
    static void onCallback(TimerHandle_t timer) {
        auto self = static_cast<FreeRTOSTimer *>( pvTimerGetTimerID(timer));
        self->_callback();
    }

public:
    FreeRTOSTimer() = default;

    void attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) override;

    ~FreeRTOSTimer() override;
};
