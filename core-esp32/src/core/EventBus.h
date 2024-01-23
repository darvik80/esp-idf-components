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
#include <list>
#include <string>
#include <string_view>
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

#include <system_error>

enum class bus_error {
    ok = 0,
    invalid_arguments,
    not_enough_memory,
};

enum class bus_condition {
    success,
    memory_failed,
    user_failed,
};

namespace std {
    template<>
    struct is_error_code_enum<bus_error> : true_type {
    };
    template<>
    struct is_error_condition_enum<bus_condition> : true_type {
    };
}

std::error_code make_error_code(bus_error);

ESP_EVENT_DECLARE_BASE(CORE_EVENT);

struct BusOptions {
    bool useSystemQueue{false};
    int32_t queueSize{32};
    uint32_t stackSize{4096};
    size_t priority{tskIDLE_PRIORITY};
    std::string name;
};

typedef std::function<void(BusOptions &options)> BusOption;

BusOption withSystemQueue(bool useSystemQueue);

BusOption withQueueSize(int32_t queueSize);

BusOption withStackSize(size_t stackSize);

BusOption withPrioritySize(size_t priority);

BusOption withName(std::string_view name);

class EventBus {
    std::list<EventSubscriber::Ptr> _subscribers;
protected:
    void doEvent(uint16_t id, Event &msg) {
        for (const auto &sub: _subscribers) {
            sub->onEventHandle(id, msg);
        }
    }

public:
    template<typename T>
    void subscribe(std::function<void(const T &msg)> callback) {
        _subscribers.push_back(std::make_shared<TFuncEventSubscriber<T>>(callback));
    }

    void subscribe(const EventSubscriber::Ptr &subscriber) {
        _subscribers.push_back(subscriber);
    }
};

template<size_t itemSize = 32>
class FreeRTOSEventBus : public EventBus {
    struct Item {
        uint16_t eventId{0};
        union {
            uint16_t all;
            struct {
                bool pointer: 1;
            };
        } flags{};
        union { ;
            uint8_t data[itemSize]{0};
            Event *ptr;
        } payload{};
    };

    QueueHandle_t _queue{nullptr};
    TaskHandle_t _task{nullptr};
private:
    static void task(void *args) {
        auto *self = static_cast<FreeRTOSEventBus *>(args);
        self->process();
    }

    void process() {
        Item item;
        while (xQueueReceive(_queue, &item, portMAX_DELAY)) {
            if (item.flags.pointer) {
                esp_logd(bus, "recv pointer 0x%04x", item.eventId);
                doEvent(item.eventId, *item.payload.ptr);
                delete item.payload.ptr;
            } else {
                esp_logd(bus, "recv copyable 0x%04x", item.eventId);
                doEvent(item.eventId, (Event &) item.payload);
            }
        }
    }

public:
    FreeRTOSEventBus(std::initializer_list<BusOption> opts) {
        BusOptions options;
        for (const auto &opt: opts) {
            opt(options);
        }

        esp_logi(bus, "create queue, size: %" PRIi32 ", item-size: %zu", options.queueSize, itemSize);
        _queue = xQueueCreate(options.queueSize, sizeof(Item));

        auto res = xTaskCreate(
                task,
                options.name.data(),
                options.stackSize,
                this,
                options.priority,
                &_task
        );

        ESP_ERROR_CHECK(res == pdPASS ? ESP_OK : ESP_ERR_INVALID_ARG);
    }

    template<typename T, std::enable_if_t<sizeof(T) <= itemSize && std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        Item item{.eventId = T::ID, .flags {.pointer = false}};
        memcpy(item.payload.data, &msg, sizeof(T));
        return xQueueSendToBack(_queue, &item, portMAX_DELAY) == pdTRUE ? ESP_OK : ESP_FAIL;
    }

    template<typename T, std::enable_if_t<(sizeof(T) > itemSize) && std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        Item item{.eventId = T::ID, .flags {.pointer = true}, .payload {.ptr = new T(msg)}};
        return xQueueSendToBack(_queue, &item, portMAX_DELAY) == pdTRUE ? ESP_OK : ESP_FAIL;
    }

    template<typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t send(T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        if (strcmp(pcTaskGetName(nullptr), pcTaskGetName(_task)) != 0) {
            esp_loge(bus, "can't send - incorrect task: 0x%04x, task: '%s'", T::ID, pcTaskGetName(nullptr));
            return ESP_ERR_INVALID_ARG;
        }

        doEvent(T::ID, msg);
        return ESP_OK;
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        esp_loge(bus, "can't post - non-copyable msg: 0x%04x", T::ID);
        return ESP_ERR_INVALID_ARG;
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t send(T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        esp_loge(bus, "can't send - non-copyable msg: 0x%04x", T::ID);
        return ESP_ERR_INVALID_ARG;
    }

    ~FreeRTOSEventBus() {
        vQueueDelete(_queue);
        if (_task) {
            vTaskDelete(_task);
        }
    }
};

class EspEventBus : public EventBus {
    std::string _task;
    esp_event_loop_handle_t _eventLoop{};
private:
    static void eventLoop(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
        auto self = (EspEventBus *) handler_arg;
        self->doEvent(id, *(Event *) event_data);
    }

public:
    EspEventBus(std::initializer_list<BusOption> opts) {
        BusOptions options;
        for (const auto &opt: opts) {
            opt(options);
        }

        if (options.useSystemQueue) {
            _task = "sys_evt";
            esp_event_loop_create_default();
            esp_event_handler_register(CORE_EVENT, ESP_EVENT_ANY_ID, eventLoop, this);
        } else {
            _task = options.name;
            esp_event_loop_args_t loop_args = {
                    .queue_size = options.queueSize,
                    .task_name = options.name.c_str(),
                    .task_priority = options.priority,
                    .task_stack_size = options.stackSize,
                    .task_core_id = 0
            };

            ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &_eventLoop));
            ESP_ERROR_CHECK(esp_event_handler_register_with(_eventLoop, CORE_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
        }
    }

    template<typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        return esp_event_post(CORE_EVENT, T::ID, &msg, sizeof(T), portMAX_DELAY);
    }

    template<typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t send(T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        if (_task == pcTaskGetName(nullptr)) {
            esp_loge(bus, "can't send - incorrect task: 0x%04x, task: '%s'", T::ID, pcTaskGetName(nullptr));
            return ESP_ERR_INVALID_ARG;
        }

        doEvent(T::ID, msg);
        return ESP_OK;
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t post(const T &) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        esp_loge(bus, "can't post - non-copyable msg: 0x%04x", T::ID);
        return ESP_ERR_INVALID_ARG;
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    esp_err_t send(T &) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from Event");
        esp_loge(bus, "can't send - non-copyable msg: 0x%04x", T::ID);
        return ESP_ERR_INVALID_ARG;
    }
};

#ifdef CONFIG_BUS_FREE_RTOS_ENABLED
typedef FreeRTOSEventBus<32> DefaultEventBus;
#else
typedef EspEventBus DefaultEventBus;
#endif

DefaultEventBus &getDefaultEventBus();

#include <freertos/message_buffer.h>

template<typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, bool> = true>
class FreeRTOSMessageBus {
    MessageBufferHandle_t _handler{nullptr};
    TaskHandle_t _task{nullptr};
private:
    static void task(void *args) {
        auto *self = static_cast<FreeRTOSMessageBus *>(args);
        self->process();
    }

    void process() {
        T item;
        while (xMessageBufferReceive(_handler, sizeof(T), &item, portMAX_DELAY) == sizeof(T)) {
            if (item.flags.pointer) {
                esp_logd(bus, "recv pointer 0x%04x", item.eventId);
                doEvent(item.eventId, *item.payload.ptr);
                delete item.payload.ptr;
            } else {
                esp_logd(bus, "recv copyable 0x%04x", item.eventId);
                doEvent(item.eventId, (Event &) item.payload);
            }
        }
    }

public:
    FreeRTOSMessageBus(std::initializer_list<BusOption> opts) {
        BusOptions options;
        for (const auto &opt: opts) {
            opt(options);
        }

        _handler = xMessageBufferCreate(sizeof(T));

        auto res = xTaskCreate(
                task,
                options.name.data(),
                options.stackSize,
                this,
                options.priority,
                &_task
        );

        ESP_ERROR_CHECK(res == pdPASS ? ESP_OK : ESP_ERR_INVALID_ARG);

    }

    esp_err_t post(const T &msg, TickType_t ticksWait) {
        return xMessageBufferSend(_handler, &msg, sizeof(T), ticksWait) == sizeof(T) ? ESP_OK : ESP_ERR_INVALID_SIZE;
    }

    ~FreeRTOSMessageBus() {
        vMessageBufferDelete(_handler);
        if (_task) {
            vTaskDelete(_task);
        }
    }
};