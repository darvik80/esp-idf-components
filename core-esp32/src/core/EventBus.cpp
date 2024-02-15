//
// Created by Ivan Kishchenko on 04/09/2023.
//

#include "EventBus.h"

ESP_EVENT_DEFINE_BASE(CORE_EVENT);

DefaultEventBus &getDefaultEventBus() {
    static DefaultEventBus bus{
            {
#ifdef CONFIG_BUS_FREE_RTOS_ENABLED
#ifdef CONFIG_BUS_FREE_RTOS_QUEUE_SIZE
                    .queueSize = CONFIG_BUS_FREE_RTOS_QUEUE_SIZE,
#endif
#ifdef CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE
                    .stackSize = CONFIG_BUS_FREE_RTOS_TASK_STACK_SIZE,
#endif
#endif
#ifdef CONFIG_BUS_ESP_EVENT_LOOP_ENABLED
#ifdef CONFIG_ESP_EVENT_QUEUE_SIZE
                    .queueSize = CONFIG_ESP_EVENT_QUEUE_SIZE,
#endif
#ifdef CONFIG_ESP_EVENT_TASK_STACK_SIZE
                    .stackSize = CONFIG_ESP_EVENT_TASK_STACK_SIZE,
#endif
#endif
                    .name = "main"
            }
    };

    return bus;
}

