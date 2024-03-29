//
// Created by Ivan Kishchenko on 09/08/2023.
//

#include <cstring>
#include "Timer.h"
#include "core/system/SystemEvent.h"

#ifndef CONFIG_IDF_TARGET_LINUX

EspTimer::EspTimer() : _name("Timer") {}

EspTimer::EspTimer(std::string_view name) : _name(name.data()) {}

void EspTimer::attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) {
    _callback = callback;

    esp_timer_create_args_t _timerConfig;
    _timerConfig.arg = reinterpret_cast<void *>(this);
    _timerConfig.callback = onCallback;
    _timerConfig.dispatch_method = ESP_TIMER_TASK;
    _timerConfig.name = _name.c_str();
    if (_timer) {
        detach();
    }
    esp_timer_create(&_timerConfig, &_timer);
    if (repeat) {
        esp_timer_start_periodic(_timer, milliseconds * 1000ULL);
    } else {
        esp_timer_start_once(_timer, milliseconds * 1000ULL);
    }
}

void EspTimer::detach() {
    if (_timer) {
        esp_timer_stop(_timer);
        esp_timer_delete(_timer);
        _timer = nullptr;
    }
}

#endif

void FreeRTOSTimer::attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) {
    _callback = callback;
    _timer = xTimerCreate(
            "timer",
            pdMS_TO_TICKS(milliseconds),
            repeat,
            this,
            FreeRTOSTimer::onCallback
    );

    xTimerStart(_timer, 0);
}

FreeRTOSTimer::~FreeRTOSTimer() {
    xTimerStop(_timer, 0);
    xTimerDelete(_timer, 0);
}
