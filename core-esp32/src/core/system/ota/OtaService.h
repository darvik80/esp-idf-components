//
// Created by Ivan Kishchenko on 29/4/24.
//

#pragma once

#include "core/system/System.h"
#include "core/Task.h"

class OtaService : public TService<OtaService, Service_Sys_Ota, Sys_Core>, public TMessageSubscriber<OtaService, OtaUpdate>{
    FreeRTOSTask _task;
public:
    explicit OtaService(Registry &registry) : TService(registry) {}

    void setup() override;

    [[nodiscard]] std::string_view getServiceName() const override {
        return "ota";
    }

    void handle(const OtaUpdate& event);
};
