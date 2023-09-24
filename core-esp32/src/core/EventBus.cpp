//
// Created by Ivan Kishchenko on 04/09/2023.
//

#include "EventBus.h"

ESP_EVENT_DEFINE_BASE(CORE_EVENT);

DefaultEventBus& getDefaultEventBus() {
    static DefaultEventBus bus;

    return bus;
}