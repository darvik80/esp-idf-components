//
// Created by Ivan Kishchenko on 04/09/2023.
//


#pragma once

#include <cstdint>
#include <string_view>
#include <optional>
#include <cJSON.h>

#include "SystemService.h"
#include "core/EventBus.h"

enum SystemEventId {
    SysEvtId_Timer,
    SysEvtId_Command,
    SysEvtId_StatusChanged,
    SysEvtId_Telemetry,
};

enum SysTimerId {
    SysTid_Telemetry,
    SysTid_User,
    SysTid_TimerMaxId,
};

template<uint8_t timerId>
struct TimerEvent : TMessage<SysEvtId_Timer, Sys_Core, timerId> {
    enum {
        TimerId = timerId
    };
};

struct Command : TMessage<SysEvtId_Command, Sys_Core> {
    char cmd[17]{0};
    char params[32]{0};
};

enum class SystemStatus {
    Sys_Unknown,
    Wifi_Connected,
    Wifi_Disconnected,
    Wifi_ScanDone,
    Wifi_ScanRequest,

    Mqtt_Connected,
    Mqtt_Reconnect,
    Mqtt_Disconnected,
};

struct SystemEventChanged : TMessage<SysEvtId_StatusChanged, Sys_Core> {
    SystemStatus status{SystemStatus::Sys_Unknown};
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

struct Telemetry : TMessage<SysEvtId_Telemetry, Sys_Core> {
    size_t freeHeap{};
    double usedMemPercent{};
    size_t stackWatermark{};
    std::optional<float> temperature{0};
    int wifiRssi{};
};

inline void toJson(cJSON *json, const Telemetry &msg) {
    cJSON_AddNumberToObject(json, "free-heap", msg.freeHeap);
    cJSON_AddNumberToObject(json, "used-mem-percent", msg.usedMemPercent);
    cJSON_AddNumberToObject(json, "stack-watermark", msg.stackWatermark);
    if (msg.temperature) {
        cJSON_AddNumberToObject(json, "temperature", msg.temperature.value());
    }
    cJSON_AddNumberToObject(json, "wifi-rssi", msg.wifiRssi);
}
