//
// Created by Ivan Kishchenko on 10/02/2024.
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <functional>
#include <list>
#include <utility>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/message_buffer.h>
#include <freertos/timers.h>
#include <core/Logger.h>

#if defined (CONFIG_IDF_TARGET)

#include <esp_event.h>

#endif

#define CONFIG_FREERTOS_ITEM_SIZE 64

namespace freertos {
    class Timer {
    public:
        Timer() = default;

        Timer(Timer &other) = delete;

        Timer(Timer &&other);

        Timer &operator=(Timer &&other);

        bool attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback);

        template<uint16_t tid>
        void fire(uint32_t milliseconds, bool repeat) {
            attach(milliseconds, repeat, []() {
                //TimerMessage<tid> event;
                //getDefaultMessageBus().post(event);
            });
        }

        void shutdown();

        ~Timer();

    private:
        static void onCallback(TimerHandle_t timer) {
            auto self = static_cast<Timer *>( pvTimerGetTimerID(timer));
            self->_callback();
        }

    private:
        TimerHandle_t _timer{};

        std::function<void()> _callback;
    };

    struct TaskOptions {
        std::function<void(void)> entryPoint;
        uint32_t stackSize = configMINIMAL_STACK_SIZE;
        size_t priority = tskIDLE_PRIORITY;
        BaseType_t coreId = tskNO_AFFINITY;
    };

    class FreeRTOSTask {
        TaskHandle_t _handle{};
        std::function<void(void)> _entryPoint{};
    private:
        static void taskEntryPoint(void *parameters) {
            auto self = static_cast<FreeRTOSTask *>(parameters);
            self->entryPoint();
        }

        void entryPoint() {
            if (_entryPoint) {
                _entryPoint();
            }
            shutdown();
        }

    public:
        FreeRTOSTask(std::string_view name, const TaskOptions &options) {
            _entryPoint = options.entryPoint;
            xTaskCreatePinnedToCore(
                    taskEntryPoint,
                    name.data(),
                    options.stackSize,
                    this,
                    options.priority,
                    &_handle,
                    options.coreId
            );
        }

        void suspend() {
            vTaskSuspend(_handle);
        }

        void resume() {
            vTaskResume(_handle);
        }

        void shutdown() {
            vTaskDelete(_handle);
        }

        ~FreeRTOSTask() {
            shutdown();
        }
    };

    struct QueueOptions {
        uint32_t queueSize{32};
    };

    template<typename T>
    class FreeRTOSQueue {
        QueueHandle_t _handle;
    public:
        explicit FreeRTOSQueue(const QueueOptions& options) {
            _handle = xQueueCreate(options.queueSize, sizeof(T));
        }

        bool send(const T &item, int ms = 0) {
            return pdPASS == xQueueSend(_handle, &item, ms ? pdMS_TO_TICKS(ms) : portMAX_DELAY);
        }

        bool sendISR(const T &item) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            return pdPASS == xQueueSendFromISR(_handle, &item, &xHigherPriorityTaskWoken);
        }

        bool receive(T &item, int ms = 0) {
            return pdPASS == xQueueReceive(_handle, &item, ms ? pdMS_TO_TICKS(ms) : portMAX_DELAY);
        }

        [[noreturn]] void process(const std::function<void(const T &item)> &callback) {
            T item{};
            while (receive(item)) {
                callback(item);
            }
        }

        ~FreeRTOSQueue() {
            vQueueDelete(_handle);
        }
    };

    template<typename T, size_t bufferSize>
    class TMessageBuffer {
        MessageBufferHandle_t _handle;
    public:
        TMessageBuffer() {
            static_assert(sizeof(T) <= bufferSize, "Msg size must be greater than or equal to buffer size");
            _handle = xMessageBufferCreate(bufferSize);
        }

        bool send(const T &item, int ms = 0) {
            return pdPASS == xMessageBufferSend(_handle, &item, sizeof(item), ms ? pdMS_TO_TICKS(ms) : portMAX_DELAY);
        }

        bool sendISR(const T &item) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            return pdPASS == xMessageBufferSendFromISR(_handle, &item, sizeof(item), &xHigherPriorityTaskWoken);
        }

        bool receive(T &item, int ms = 0) {
            return pdPASS == xMessageBufferReceive(_handle, &item, sizeof(T), ms ? pdMS_TO_TICKS(ms) : portMAX_DELAY);
        }

        [[noreturn]] void process(const std::function<void(const T &item)> &callback) {
            T item{};
            while (receive(item)) {
                callback(item);
            }
        }

        ~TMessageBuffer() {
            vMessageBufferDelete(_handle);
        }
    };
}

namespace experimental {
    typedef uint16_t MessageId;

    class IMessage {
    public:
        [[nodiscard]] virtual MessageId id() const = 0;

        virtual IMessage *clone() const = 0;

        virtual ~IMessage() = default;
    };

    template<typename T, uint8_t msgId, uint8_t sysId>
    class TMessage : public IMessage {
    public:
        enum {
            ID = ((uint16_t) msgId & 0xFF) | (((uint16_t) sysId & 0xFF) << 8)
        };

        [[nodiscard]] MessageId id() const override {
            return ID;
        }

        virtual IMessage *clone() const {
            return new T(*((T *) this));
        }
    };

    class MessageSubscriber {
    public:
        typedef std::shared_ptr<MessageSubscriber> Ptr;

        virtual void onMessage(const IMessage &message) = 0;

        virtual ~MessageSubscriber() = default;
    };

    template<typename T, typename ...Msgs>
    class TMessageSubscriber : public MessageSubscriber {
    private:
        template<typename Msg>
        bool onMessage(const IMessage &msg) {
            if (Msg::ID == msg.id()) {
                static_cast<T *>(this)->handle(static_cast<const Msg &>(msg));
                return true;
            }

            return false;
        }

    public:
        void onMessage(const IMessage &msg) override {
            (onMessage<Msgs>(msg) || ...);
        }
    };

    template<typename T>
    class TFuncMessageSubscriber : public TMessageSubscriber<TFuncMessageSubscriber<T>, T> {
        std::function<void(const T &msg)> _callback;
    public:
        explicit TFuncMessageSubscriber(const std::function<void(const T &)> &callback)
                : _callback(callback) {}

        void onMessage(const T &msg) {
            _callback(msg);
        }
    };

    class IMessageBus {
        std::list<MessageSubscriber::Ptr> _subscribers;
    protected:
        void receive(const IMessage &msg) {
            for (auto &subscriber: _subscribers) {
                subscriber->onMessage(msg);
            }
        }

        virtual bool isSameTask() = 0;

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

        virtual bool post(const IMessage &msg) = 0;

        bool send(const IMessage &msg) {
            if (!isSameTask()) {
                esp_loge(bus, "can't send - incorrect task: 0x%04x, task: '%s'", msg.id(), pcTaskGetName(nullptr));
                return false;
            }

            receive(msg);
            return true;
        }
    };

    struct MessageBusOptions {
        uint32_t stackSize = CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE;
        size_t priority = tskIDLE_PRIORITY;
        int32_t queueSize = CONFIG_BUS_FREE_RTOS_QUEUE_SIZE;
        bool useSystemQueue = false;
    };

    class FreeRTOSMessageBus : public IMessageBus {
        TaskHandle_t _task{};
        QueueHandle_t _queue{};
        std::list<MessageSubscriber::Ptr> _subscribers;
    private:
        static void taskEntryPoint(void *args) {
            auto self = static_cast<FreeRTOSMessageBus *>(args);
            self->process();
        }

        void process() {
            IMessage *msg;
            while (pdPASS == xQueueReceive(_queue, &msg, portMAX_DELAY)) {
                receive(*msg);
                delete msg;
            }
        }

        bool isSameTask() override {
            return 0 == strcmp(pcTaskGetName(nullptr), pcTaskGetName(_task));
        }

    public:
        FreeRTOSMessageBus() = delete;

        FreeRTOSMessageBus(FreeRTOSMessageBus &) = delete;

        explicit FreeRTOSMessageBus(std::string_view name, const MessageBusOptions &options) {
            _queue = xQueueCreate(options.queueSize, sizeof(IMessage *));
            xTaskCreate(FreeRTOSMessageBus::taskEntryPoint, name.data(), options.stackSize, this, options.priority,
                        &_task);
        }

        bool post(const IMessage &msg) override {
            auto ptr = msg.clone();
            return pdPASS == xQueueSend(_queue, &ptr, portMAX_DELAY);
        }

        ~FreeRTOSMessageBus() {
            vTaskDelete(_task);
            vQueueDelete(_queue);
        }
    };

#if defined (CONFIG_IDF_TARGET)

    ESP_EVENT_DECLARE_BASE(MAIN_EVENT);

    class EspIdfEventBus : public IMessageBus {
        std::string _task;
        esp_event_loop_handle_t _eventLoop{};
        std::list<MessageSubscriber::Ptr> _subscribers;
    private:
        static void eventLoop(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
            auto self = (EspIdfEventBus *) handler_arg;
            self->receive(*(IMessage *) event_data);
        }

    public:
        explicit EspIdfEventBus(std::string_view name, const MessageBusOptions &options) {
            _task = name;
            if (options.useSystemQueue) {
                esp_event_loop_create_default();
                esp_event_handler_register(MAIN_EVENT, ESP_EVENT_ANY_ID, eventLoop, this);
            } else {
                esp_event_loop_args_t loop_args = {
                        .queue_size = options.queueSize,
                        .task_name = name.data(),
                        .task_priority = options.priority,
                        .task_stack_size = options.stackSize,
                        .task_core_id = 0
                };

                ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &_eventLoop));
                ESP_ERROR_CHECK(
                        esp_event_handler_register_with(_eventLoop, MAIN_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
            }
        }

        bool post(const IMessage &msg) override {
            auto ptr = msg.clone();
            return ESP_OK == esp_event_post(MAIN_EVENT, msg.id(), &ptr, sizeof(IMessage *), portMAX_DELAY);
        }

        ~EspIdfEventBus() {
            if (_eventLoop) {
                esp_event_loop_delete(_eventLoop);
            }
        }
    };

#endif

    IMessageBus &getDefaultMessageBus();

    template<typename T, uint8_t msgId, uint8_t sysId, uint8_t version = 0>
    struct Trivial {
        enum {
            ID = ((uint32_t) msgId | (((uint32_t) sysId & 0xFF) << 8) | (((uint32_t) version & 0xFF) << 16))
        };

        static T *cast(void *ptr) {
            return static_cast<T *>(ptr);
        }
    };

    struct TrivialMessageBusItem {
        uint32_t id{};
        union {
            uint8_t all;
            struct {
                bool pointer: 1;
            };
        } flags{};
        union { ;
            uint8_t data[CONFIG_FREERTOS_ITEM_SIZE]{0};

            struct {
                void *ptr;

                void (*destroy)(void *ptr);
            };
        } payload{};
    };

}

