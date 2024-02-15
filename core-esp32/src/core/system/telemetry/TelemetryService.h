//
// Created by Ivan Kishchenko on 21/09/2023.
//

#pragma once

#include <sdkconfig.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
#include <driver/temperature_sensor.h>
#endif

#include "core/system/System.h"

class TelemetryService: public TService<TelemetryService, Service_Sys_Telemetry, Sys_Core>
        , public TMessageSubscriber<TelemetryService, TimerEvent<SysTid_Telemetry>> {
private:
    FreeRTOSTimer _timer;
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
    temperature_sensor_handle_t _temp_handle;
#endif
public:
    explicit TelemetryService(Registry &registry);
    void handle(const TimerEvent<SysTid_Telemetry>&);
    ~TelemetryService() override;
};
