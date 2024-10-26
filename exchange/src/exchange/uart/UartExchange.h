//
// Created by Ivan Kishchenko on 29/8/24.
//

#pragma once

#include <sdkconfig.h>

#ifdef CONFIG_EXCHANGE_BUS_UART

#include <core/Registry.h>
#include <core/system/SystemService.h>
#include <exchange/Exchange.h>

class UartExchange : public AbstractExchange<Service_Sys_UartExchange> {
public:
    explicit UartExchange(Registry &registry) : AbstractExchange(registry) {}

    [[nodiscard]] std::string_view getServiceName() const override {
        return "uart-exchange";
    }

    void setup() override;
};

#endif
