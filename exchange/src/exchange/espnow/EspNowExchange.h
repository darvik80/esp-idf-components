//
// Created by Ivan Kishchenko on 4/10/24.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_EXCHANGE_BUS_ESP_NOW

#include <exchange/Exchange.h>

class EspNowExchange : public AbstractExchange<Service_Sys_EspNowExchange> {

public:
    explicit EspNowExchange(Registry &registry) : AbstractExchange(registry) {}

    [[nodiscard]] std::string_view getServiceName() const override {
        return "esp-now-exchange";
    }

    void setup() override;
};

#endif
