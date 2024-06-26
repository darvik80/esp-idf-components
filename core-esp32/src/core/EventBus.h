//
// Created by Ivan Kishchenko on 04/09/2023.
//

#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>
#include <functional>
#include <cstring>
#include <memory>
#include <list>
#include <string>
#include <string_view>

#include "Logger.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#if defined (CONFIG_IDF_TARGET)

#include <esp_check.h>
#include <esp_event.h>
#include <esp_log.h>

#endif

typedef uint32_t MessageId;

constexpr MessageId genMsgId(uint8_t id, uint8_t  sysId, uint16_t params) {
    return (uint32_t)id | (((uint32_t)sysId & 0xFF) << 8) | (((uint32_t)params & 0xFF) << 16);
}

struct SMessage {
};

template<uint8_t eventId, uint8_t sysId, uint16_t bits = 0>
struct TMessage : SMessage {
    enum {
        ID = genMsgId(eventId, sysId, bits)
    };
};

struct IMessage {
    [[nodiscard]] virtual MessageId id() const = 0;

    virtual ~IMessage() = default;
};

template<uint8_t eventId, uint8_t sysId, uint8_t bits = 0>
struct CMessage : IMessage {
    enum {
        ID = genMsgId(eventId, sysId, bits)
    };

    [[nodiscard]] MessageId id() const override {
        return ID;
    }
};

class MessageSubscriber {
public:
    typedef std::shared_ptr<MessageSubscriber> Ptr;

    virtual void onMessage(MessageId msgId, const void *msg) = 0;

    virtual ~MessageSubscriber() = default;
};

template<typename T, typename ...Msgs>
class TMessageSubscriber : public MessageSubscriber {
private:
    template<typename Msg>
    bool onMessage(MessageId msgId, const void *msg) {
        if (Msg::ID == msgId) {
            static_cast<T *>(this)->handle(*(reinterpret_cast<const Msg *>(msg)));
            return true;
        }

        return false;
    }

public:
    void onMessage(MessageId msgId, const void *msg) override {
        (onMessage<Msgs>(msgId, msg) || ...);
    }
};

template<typename T>
class TFuncMessageSubscriber : public TMessageSubscriber<TFuncMessageSubscriber<T>, T> {
    std::function<void(const T &msg)> _callback;
public:
    explicit TFuncMessageSubscriber(const std::function<void(const T &)> &callback)
            : _callback(callback) {}

    void handle(const T &msg) {
        _callback(msg);
    }
};

struct BusOptions {
    bool useSystemQueue{false};
    int32_t queueSize{32};
    uint32_t stackSize = configMINIMAL_STACK_SIZE;
    size_t priority{tskIDLE_PRIORITY};
    std::string name;
};

class EventBus {
    std::list<MessageSubscriber::Ptr> _subscribers;

protected:
    void handleMessage(MessageId id, const void *msg) {
        for (const auto &sub: _subscribers) {
            sub->onMessage(id, msg);
        }
    }

public:
    template<typename T>
    void subscribe(std::function<void(const T &msg)> callback) {
        _subscribers.push_back(std::make_shared<TFuncMessageSubscriber<T>>(callback));
    }

    void subscribe(const MessageSubscriber::Ptr &subscriber) {
        _subscribers.push_back(subscriber);
    }

    void unsubscribe(const MessageSubscriber::Ptr &subscriber) {
        _subscribers.remove(subscriber);
    }
};

template<size_t itemSize = 32>
class FreeRTOSEventBus : public EventBus {
    struct Item {
        MessageId messageId{0};
        union {
            uint8_t all;
            struct {
                bool pointer: 1;
                bool trivial: true;
            };
        } flags{};
        union { ;
            uint8_t data[itemSize]{0};
            SMessage* trivial;
            IMessage* non_trivial;
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
                esp_logd(bus, "recv pointer 0x%04" PRIxLEAST32, item.messageId);
                if (item.flags.trivial) {
                    handleMessage(item.messageId, item.payload.trivial);
                    free(item.payload.trivial);
                } else {
                    handleMessage(item.messageId, item.payload.non_trivial);
                    delete item.payload.non_trivial;
                }
            } else {
                esp_logd(bus, "recv copyable 0x%04" PRIxLEAST32, item.messageId);
                handleMessage(item.messageId, &item.payload.data);
            }
        }
    }

public:
    explicit FreeRTOSEventBus(const BusOptions &options) {
        esp_logi(bus, "create freertos queue: " LOG_COLOR_I "%s" LOG_RESET_COLOR ", size: %" PRIi32 ", item-size: %zu",
                 options.name.c_str(), options.queueSize, itemSize);
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
    bool post(const T &msg) {
        static_assert((std::is_base_of<SMessage, T>::value), "Msg is not derived from trivial message");
        Item item{
                .messageId = T::ID,
                .flags {.pointer = false, .trivial = true}
        };
        memcpy(item.payload.data, &msg, sizeof(T));
        return xQueueSendToBack(_queue, &item, portMAX_DELAY) == pdTRUE;
    }

    template<typename T, std::enable_if_t<(sizeof(T) > itemSize) && std::is_trivially_copyable<T>::value, bool> = true>
    bool post(const T &msg) {
        static_assert((std::is_base_of<SMessage, T>::value), "Msg is not derived from trivial message");
        Item item{
                .messageId = T::ID,
                .flags {.pointer = true, .trivial = true},
                .payload {.trivial = (SMessage*)memcpy(malloc(sizeof(T)), &msg, sizeof(T))}
        };
        return xQueueSendToBack(_queue, &item, portMAX_DELAY) == pdTRUE;
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    bool post(const T &msg) {
        static_assert((std::is_base_of<IMessage, T>::value), "Msg is not derived from not trivial message");
        Item item{
                .messageId = T::ID,
                .flags {.pointer = true, .trivial = false},
                .payload {.non_trivial = new T(msg)}
        };
        return xQueueSendToBack(_queue, &item, portMAX_DELAY) == pdTRUE;
    }

    template<typename T>
    bool send(const T &msg) {
        static_assert((std::is_base_of<SMessage, T>::value || std::is_base_of<IMessage, T>::value),
                      "Msg is not derived from message");
        if (strcmp(pcTaskGetName(nullptr), pcTaskGetName(_task)) != 0) {
            esp_loge(bus, "can't send - incorrect task: 0x%04x, task: '%s'", T::ID, pcTaskGetName(nullptr));
            return false;
        }

        handleMessage(T::ID, &msg);
        return true;
    }

    ~FreeRTOSEventBus() {
        vQueueDelete(_queue);
        if (_task) {
            vTaskDelete(_task);
        }
    }
};

#if defined (CONFIG_IDF_TARGET)

ESP_EVENT_DECLARE_BASE(CORE_EVENT);
ESP_EVENT_DECLARE_BASE(CORE_NT_EVENT);

class EspEventBus : public EventBus {
    std::string _task;
    esp_event_loop_handle_t _eventLoop{};
private:
    static void eventLoop(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
        auto self = static_cast<EspEventBus *>( handler_arg);
        if (base == CORE_NT_EVENT) {
            auto ptr = *(IMessage **) event_data;
            self->handleMessage((MessageId) id, ptr);
            delete ptr;
        } else {
            self->handleMessage((MessageId) id, event_data);
        }
    }

public:
    explicit EspEventBus(const BusOptions &options) {
        esp_logi(bus, "create esp-event-bus: " LOG_COLOR_I "%s" LOG_RESET_COLOR ", size: %" PRIi32 "",
                 options.name.c_str(), options.queueSize);
        if (options.useSystemQueue) {
            _task = "sys_evt";
            esp_event_loop_create_default();
            ESP_ERROR_CHECK(esp_event_handler_register(CORE_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
            ESP_ERROR_CHECK(esp_event_handler_register(CORE_NT_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
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
            ESP_ERROR_CHECK(esp_event_handler_register_with(_eventLoop, CORE_NT_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
        }
    }

    template<typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, bool> = true>
    bool post(const T &msg) {
        static_assert((std::is_base_of<SMessage, T>::value), "Msg is not derived from trivial message");
        return ESP_OK == esp_event_post(CORE_EVENT, T::ID, &msg, sizeof(T), portMAX_DELAY);
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    bool post(const T &msg) {
        static_assert((std::is_base_of<IMessage, T>::value), "Msg is not derived from non-trivial message");
        auto ptr = new T(msg);
        return ESP_OK == esp_event_post(CORE_NT_EVENT, T::ID, &ptr, sizeof(T *), portMAX_DELAY);
    }

    template<typename T>
    bool send(const T &msg) {
        static_assert((std::is_base_of<SMessage, T>::value || std::is_base_of<IMessage, T>::value),
                      "Msg is not derived from message");
        if (_task != pcTaskGetName(nullptr)) {
            esp_loge(bus, "can't send - incorrect task: 0x%04x, task: '%s'", T::ID, pcTaskGetName(nullptr));
            return false;
        }

        handleMessage(T::ID, &msg);
        return true;
    }
};

#ifdef CONFIG_BUS_FREE_RTOS_ENABLED
typedef FreeRTOSEventBus<32> DefaultEventBus;
#else
typedef EspEventBus DefaultEventBus;
#endif

DefaultEventBus &getDefaultEventBus();

#endif