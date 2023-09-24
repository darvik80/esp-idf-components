//
// Created by Ivan Kishchenko on 08/08/2023.
//

#pragma once

#include <cstring>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <set>
#include "cJSON.h"
#include "Core.h"


struct Properties {
    typedef std::shared_ptr<Properties> Ptr;

    [[nodiscard]] virtual uint16_t getPropId() const = 0;

    virtual ~Properties() = default;
};

template<uint8_t id, System sysId = System::Sys_User>
struct TProperties : Properties {
public:
    enum {
        ID = (id | ((uint16_t) sysId) << 8)
    };

    [[nodiscard]] uint16_t getPropId() const override {
        return (id | ((uint16_t) sysId) << 8);
    }
};

struct WifiProperties : TProperties<Props_Sys_Wifi, System::Sys_Core> {
    struct AccessPoint {
        std::string ssid;
        std::string password;
    };
    std::vector<AccessPoint> ap;
};

[[maybe_unused]] void fromJson(cJSON *json, WifiProperties &props);

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

class PropertiesConsumer {
public:
    virtual void applyProperties(const Properties &props) = 0;
};

template<typename T, typename Props1, typename Props2 = void>
class TPropertiesConsumer : public PropertiesConsumer {
public:
    void applyProperties(const Properties &props) override {
        switch (props.getPropId()) {
            case Props1::ID:
                static_cast<T *>(this)->apply(static_cast<const Props1 &>(props));
                break;
            case Props2::ID:
                static_cast<T *>(this)->apply(static_cast<const Props2 &>(props));
                break;
            default:
                break;
        }
    }
};

template<typename T, typename Props1>
class TPropertiesConsumer<T, Props1, void> : public PropertiesConsumer {
public:
    void applyProperties(const Properties &props) override {
        switch (props.getPropId()) {
            case Props1::ID:
                static_cast<T *>(this)->apply(static_cast<const Props1 &>(props));
                break;
            default:
                break;
        }
    }
};

typedef std::function<Properties::Ptr(cJSON *props)> PropertiesReader;

template<typename Props>
Properties::Ptr defaultPropertiesReader(cJSON *json) {
    auto props = std::make_shared<Props>();
    fromJson(json, *props);

    return props;
}

class PropertiesLoader {
    std::map<std::string, PropertiesReader> _readers;

    std::vector<PropertiesConsumer *> _consumers;
public:
    void load(std::string_view filePath);

    void addReader(std::string_view props, const PropertiesReader &callback) {
        _readers[props.data()] = callback;
    }

    void addConsumer(PropertiesConsumer *consumer) {
        _consumers.push_back(consumer);
    }
};