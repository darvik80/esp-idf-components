//
// Created by Ivan Kishchenko on 04/09/2023.
//


#pragma once

#include <cstdint>
#include <type_traits>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_log.h>

#include <vector>
#include <functional>
#include <cstring>
#include <memory>
#include <esp_check.h>
#include <esp_event.h>
#include "Logger.h"

typedef uint16_t MsgId;

typedef uint8_t SubMsgId;

#define DEF_MSG_ID(id, sysId, params) (id | (((uint16_t)sysId & 0x03) << 8) | ((uint16_t)params << 10))

struct Event {
};

template<uint8_t eventId, uint8_t sysId, uint8_t bits = 0>
struct TEvent : Event {
    enum {
        ID = DEF_MSG_ID(eventId, sysId, bits)
    };
};

class EventSubscriber {
public:
    typedef std::shared_ptr<EventSubscriber> Ptr;

    virtual void onEventHandle(uint16_t id, const Event &msg) = 0;

    virtual ~EventSubscriber() = default;
};

template<typename T, typename Evt1 = void, typename Evt2 = void, typename Evt3 = void, typename Evt4 = void, typename Evt5 = void, typename Evt6 = void, typename Evt7 = void, typename Evt8 = void>
class TEventSubscriber : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        switch (id) {
            case Evt1::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
                break;
            case Evt2::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt2 &>(event));
                break;
            case Evt3::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt3 &>(event));
                break;
            case Evt4::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt4 &>(event));
                break;
            case Evt5::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt5 &>(event));
                break;
            case Evt6::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt6 &>(event));
                break;
            case Evt7::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt7 &>(event));
                break;
            case Evt8::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt8 &>(event));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Evt1, typename Evt2, typename Evt3, typename Evt4, typename Evt5, typename Evt6, typename Evt7>
class TEventSubscriber<T, Evt1, Evt2, Evt3, Evt4, Evt5, Evt6, Evt7> : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        switch (id) {
            case Evt1::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
                break;
            case Evt2::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt2 &>(event));
                break;
            case Evt3::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt3 &>(event));
                break;
            case Evt4::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt4 &>(event));
                break;
            case Evt5::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt5 &>(event));
                break;
            case Evt6::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt6 &>(event));
                break;
            case Evt7::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt7 &>(event));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Evt1, typename Evt2, typename Evt3, typename Evt4, typename Evt5, typename Evt6>
class TEventSubscriber<T, Evt1, Evt2, Evt3, Evt4, Evt5, Evt6> : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        switch (id) {
            case Evt1::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
                break;
            case Evt2::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt2 &>(event));
                break;
            case Evt3::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt3 &>(event));
                break;
            case Evt4::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt4 &>(event));
                break;
            case Evt5::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt5 &>(event));
                break;
            case Evt6::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt6 &>(event));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Evt1, typename Evt2, typename Evt3, typename Evt4, typename Evt5>
class TEventSubscriber<T, Evt1, Evt2, Evt3, Evt4, Evt5> : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        switch (id) {
            case Evt1::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
                break;
            case Evt2::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt2 &>(event));
                break;
            case Evt3::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt3 &>(event));
                break;
            case Evt4::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt4 &>(event));
                break;
            case Evt5::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt5 &>(event));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Evt1, typename Evt2, typename Evt3, typename Evt4>
class TEventSubscriber<T, Evt1, Evt2, Evt3, Evt4> : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        switch (id) {
            case Evt1::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
                break;
            case Evt2::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt2 &>(event));
                break;
            case Evt3::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt3 &>(event));
                break;
            case Evt4::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt4 &>(event));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Evt1, typename Evt2, typename Evt3>
class TEventSubscriber<T, Evt1, Evt2, Evt3> : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        switch (id) {
            case Evt1::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
                break;
            case Evt2::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt2 &>(event));
                break;
            case Evt3::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt3 &>(event));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Evt1, typename Evt2>
class TEventSubscriber<T, Evt1, Evt2> : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        switch (id) {
            case Evt1::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
                break;
            case Evt2::ID:
                static_cast<T *>(this)->onEvent(static_cast<const Evt2 &>(event));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Evt1>
class TEventSubscriber<T, Evt1> : public EventSubscriber {
public:
    void onEventHandle(uint16_t id, const Event &event) override {
        if (id == Evt1::ID) {
            static_cast<T *>(this)->onEvent(static_cast<const Evt1 &>(event));
        }
    }
};

template<typename T>
class TFuncEventSubscriber : public TEventSubscriber<TFuncEventSubscriber<T>, T> {
    std::function<void(const T &msg)> _callback;
public:
    explicit TFuncEventSubscriber(const std::function<void(const T &)> &callback)
            : _callback(callback) {}

    void onEvent(const T &msg) {
        _callback(msg);
    }
};

template<size_t queueSize = 32, size_t itemSize = 64>
class TEventBus {
    struct Container {
        uint16_t eventId{0};
        bool isPointer{false};
        union { ;
            uint8_t data[itemSize]{0};
            Event *ptr;
        } payload{0};
    };

    QueueHandle_t _queue{nullptr};

    std::vector<EventSubscriber::Ptr> _subscribers;
private:
    void doEvent(uint16_t id, Event &msg) {
        for (const auto &sub: _subscribers) {
            sub->onEventHandle(id, msg);
        }
    }

public:
    TEventBus() {
        esp_logi(bus, "create queue,  size: %d, item-size: %d", queueSize, sizeof(Container));
        _queue = xQueueCreate(queueSize, sizeof(Container));
    }

    void process() {
        Container container;
        while (xQueueReceive(_queue, &container, portMAX_DELAY)) {
            if (container.isPointer) {
                esp_logd(bus, "recv copyable 0x%04x", container.eventId);
                doEvent(container.eventId, *container.payload.ptr);
                delete container.payload.ptr;
            } else {
                esp_logd(bus, "recv pointer 0x%04x", container.eventId);
                doEvent(container.eventId, (Event &) container.payload);
            }
        }
    }

    template<typename T>
    void subscribe(std::function<void(const T &msg)> callback) {
        _subscribers.push_back(std::make_shared<TFuncEventSubscriber<T>>(callback));
    }

    void subscribe(const EventSubscriber::Ptr &subscriber) {
        _subscribers.push_back(subscriber);
    }

    template<typename T>
    esp_err_t send(T &msg) {
        if (strcmp(pcTaskGetName(nullptr), "main") != 0) {
            esp_loge(bus, "can't send - incorrect task: 0x%04x, task: '%s'", T::ID, pcTaskGetName(nullptr));
            return ESP_ERR_INVALID_ARG;
        }

        doEvent(T::ID, msg);
        return ESP_OK;
    }

    template<typename T, std::enable_if_t<sizeof(T) <= itemSize && std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t postISR(const T &msg) {
        esp_logd(bus, "post - copyable: 0x%04x", T::ID);
        Container container{.eventId = T::ID, .isPointer= false};
        memcpy(container.payload.data, &msg, sizeof(T));
        return xQueueSendToBackFromISR(_queue, &container, pdFALSE) == pdTRUE ? ESP_OK : ESP_FAIL;
    }

    template<typename T, std::enable_if_t<sizeof(T) <= itemSize && std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &msg) {
        esp_logd(bus, "post - copyable: 0x%04x", T::ID);
        Container container{.eventId = T::ID, .isPointer= false};
        memcpy(container.payload.data, &msg, sizeof(T));
        return xQueueSendToBack(_queue, &container, portMAX_DELAY) == pdTRUE ? ESP_OK : ESP_FAIL;
    }

    template<typename T, std::enable_if_t<(sizeof(T) > itemSize) && std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &msg) {
        esp_logd(bus, "post - pointer: 0x%04x", T::ID);
        Container container{.eventId = T::ID, .isPointer= true, .payload {.ptr = new T(msg)}};
        return xQueueSendToBack(_queue, &container, portMAX_DELAY) == pdTRUE ? ESP_OK : ESP_FAIL;
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &) {
        esp_loge(bus, "can't post - non-copyable: 0x%04x", T::ID);
        return ESP_ERR_INVALID_ARG;
    }

    ~TEventBus() {
        vQueueDelete(_queue);
    }
};

class DefaultEventBus : public TEventBus<32, 64> {
};

DefaultEventBus &getDefaultEventBus();