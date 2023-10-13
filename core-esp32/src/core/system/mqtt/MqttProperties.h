//
// Created by Ivan Kishchenko on 12/10/2023.
//

#pragma once

#include "core/system/SystemProperties.h"
#include "core/system/SystemService.h"

struct MqttProperties : TProperties<Props_Sys_Mqtt, System::Sys_Core> {
    struct BrokerInfo {
        std::string type;
        std::string uri;
        std::string username;
        std::string password;
        std::string caCert;
        std::string clientCert;
        std::string clientKey;
        std::string productName;
        std::string deviceName;
        std::string deviceSecret;
    };

    std::vector<BrokerInfo> brokers;
    int retries{3};
};

[[maybe_unused]] void fromJson(cJSON *json, MqttProperties &props);
