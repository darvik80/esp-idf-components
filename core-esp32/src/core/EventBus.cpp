//
// Created by Ivan Kishchenko on 04/09/2023.
//

#include "EventBus.h"

#ifdef CONFIG_BUS_ESP_EVENT_LOOP_ENABLED
ESP_EVENT_DEFINE_BASE(CORE_EVENT);
#endif


DefaultEventBus& getDefaultEventBus() {
    static DefaultEventBus bus;

    return bus;
}