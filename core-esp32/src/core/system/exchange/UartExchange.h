//
// Created by Ivan Kishchenko on 22/6/24.
//

#pragma once

#include "core/system/System.h"

class UartExchange : public TService<UartExchange, Service_Sys_UartExchange, Sys_Core> {
    QueueHandle_t _rxQueue;
    QueueHandle_t _txQueue;
private:
    static void rxTask(void* args) {
        auto* self = static_cast<UartExchange*>(args);
        self->rxTask();
    }

    [[noreturn]] [[noreturn]] [[noreturn]] void rxTask();
public:
    explicit UartExchange(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "uart-ex";
    }

    void setup() override;
};
