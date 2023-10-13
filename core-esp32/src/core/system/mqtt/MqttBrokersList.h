//
// Created by Ivan Kishchenko on 10/09/2023.
//

#pragma once

#include "MqttProperties.h"

class MqttBrokersList {
public:
    struct BrokerInfo {
        std::string uri;
        std::string clientId;
        std::string username;
        std::string password;
        std::string caCert;
        std::string clientCert;
        std::string clientKey;
        std::string productName;
        std::string deviceName;
    };
private:
    static BrokerInfo brokerInfoFromProperties(const MqttProperties::BrokerInfo& props);
    static void mqttPropsToBrokerList(const MqttProperties& props,  std::vector<BrokerInfo>& result);

    int _retries{0};
    int _iterRetries{0};
    std::vector<BrokerInfo>::iterator _iter;
    std::vector<BrokerInfo> _brokers;
public:
    MqttBrokersList() = default;
    explicit MqttBrokersList(const MqttProperties &props) : _retries(props.retries) {
        mqttPropsToBrokerList(props, _brokers);
        _iter = _brokers.end();

    }

    void apply(const MqttProperties &props) {
        mqttPropsToBrokerList(props, _brokers);
        _iter = _brokers.end();
        _retries = props.retries;

    }

    std::tuple<BrokerInfo &, bool> getNextBroker() {
        if (--_iterRetries <= 0) {
            _iterRetries = _retries;
            if (_iter == _brokers.end()) {
                _iter = _brokers.begin();
            } else if (++_iter == _brokers.end()) {
                _iter = _brokers.begin();
            }

            return {*_iter, true};
        }

        return {*_iter, false};
    }

};
