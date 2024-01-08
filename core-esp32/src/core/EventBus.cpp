//
// Created by Ivan Kishchenko on 04/09/2023.
//

#include "EventBus.h"

#ifdef CONFIG_BUS_ESP_EVENT_LOOP_ENABLED
ESP_EVENT_DEFINE_BASE(CORE_EVENT);
#endif


DefaultEventBus &getDefaultEventBus() {
    static DefaultEventBus bus{
#ifdef CONFIG_BUS_FREE_RTOS_ENABLED
#ifdef CONFIG_BUS_FREE_RTOS_QUEUE_SIZE
            withQueueSize(CONFIG_BUS_FREE_RTOS_QUEUE_SIZE),
#endif
#ifdef CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE
            withStackSize(CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE),
#endif
#endif
#ifdef CONFIG_BUS_ESP_EVENT_LOOP_ENABLED
#ifdef CONFIG_ESP_EVENT_QUEUE_SIZE
            withQueueSize(CONFIG_ESP_EVENT_QUEUE_SIZE),
#endif
#ifdef CONFIG_ESP_EVENT_TASK_STACK_SIZE
            withStackSize(CONFIG_ESP_EVENT_TASK_STACK_SIZE),
#endif
#endif
            withName("main")
    };


    return bus;
}

EventBusOption withSystemQueue(bool useSystemQueue) {
    return [useSystemQueue](EventBusOptions &options) {
        options.useSystemQueue = useSystemQueue;
    };
}

EventBusOption withQueueSize(int32_t queueSize) {
    return [queueSize](EventBusOptions &options) {
        options.queueSize = queueSize;
    };
}

EventBusOption withStackSize(size_t stackSize) {
    return [stackSize](EventBusOptions &options) {
        options.stackSize = stackSize;
    };
}

EventBusOption withPrioritySize(size_t priority) {
    return [priority](EventBusOptions &options) {
        options.priority = priority;
    };
}

EventBusOption withName(std::string_view name) {
    return [name](EventBusOptions &options) {
        options.name = name;
    };
}
