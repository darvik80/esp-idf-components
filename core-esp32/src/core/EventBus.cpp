//
// Created by Ivan Kishchenko on 04/09/2023.
//

#include "EventBus.h"

DefaultEventBus& getDefaultEventBus() {
    static DefaultEventBus bus;

    return bus;
}