//
// Created by Ivan Kishchenko on 04/09/2023.
//


#pragma once

#include <cstdint>
#include "core/Core.h"
#include "core/EventBus.h"
#include <string_view>
#include <optional>
#include <cJSON.h>

enum SystemEventId {
    SysEvtId_Timer,
    SysEvtId_Command,
    SysEvtId_StatusChanged,
    SysEvtId_Telemetry,
};

enum SysTimerId {
    SysTid_Telemetry,
};

template<uint16_t timerId>
struct TimerEvent : TEvent<SysEvtId_Timer, Sys_Core, timerId> {
    enum {
        TimerId = timerId
    };
};

struct Command : TEvent<SysEvtId_Command, Sys_Core> {
    char cmd[17]{0};
    char params[32]{0};
};

enum class SystemStatus {
    Wifi_Connected,
    Wifi_Disconnected,
    Wifi_ScanDone,
    Wifi_ScanRequest,

    Mqtt_Connected,
    Mqtt_Reconnect,
    Mqtt_Disconnected,
};

struct SystemEventChanged : TEvent<SysEvtId_StatusChanged, Sys_Core> {
    SystemStatus status;
};

inline void toJson(cJSON *json, const SystemEventChanged &msg) {
    std::string_view status;
    switch (msg.status) {
        case SystemStatus::Wifi_Connected:
            status = "wifi-connected";
            break;
        case SystemStatus::Wifi_Disconnected:
            status = "wifi-disconnected";
            break;
        case SystemStatus::Mqtt_Connected:
            status = "mqtt-connected";
            break;
        case SystemStatus::Mqtt_Disconnected:
            status = "mqtt-disconnected";
            break;
        default:
            break;
    }
    cJSON_AddStringToObject(json, "status", status.data());
}

struct Telemetry : TEvent<SysEvtId_Telemetry, Sys_Core> {
    uint32_t freeHeap{};
    uint32_t stackWatermark{};
    std::optional<float> temperature{0};
};

inline void toJson(cJSON *json, const Telemetry &msg) {
    cJSON_AddNumberToObject(json, "free-heap", msg.freeHeap);
    cJSON_AddNumberToObject(json, "stack-watermark", msg.stackWatermark);
    if (msg.temperature) {
        cJSON_AddNumberToObject(json, "temperature", msg.temperature.value());
    }
}
