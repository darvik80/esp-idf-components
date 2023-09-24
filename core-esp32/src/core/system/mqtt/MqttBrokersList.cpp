//
// Created by Ivan Kishchenko on 10/09/2023.
//

#include "MqttBrokersList.h"
#include "AlibabaIotCredentials.h"

MqttBrokersList::BrokerInfo MqttBrokersList::brokerInfoFromProperties(const MqttProperties::BrokerInfo &props) {
    if (props.type == "alicloud") {
        AlibabaIotCredentials credentials(props.productName, props.deviceName, props.deviceSecret);

        return MqttBrokersList::BrokerInfo{
                .uri = credentials.uri().data(),
                .clientId = credentials.clientId().data(),
                .username = credentials.username().data(),
                .password = credentials.password().data(),
                .caCert = credentials.caCertificate().data(),
                .productName = props.productName,
                .deviceName = props.deviceName,
        };
    }
    return MqttBrokersList::BrokerInfo{
            .uri = props.uri,
            .clientId = props.deviceName,
            .username = props.username,
            .password = props.password,
            .caCert = props.caCert,
            .clientCert = props.clientCert,
            .clientKey = props.clientKey,
            .productName = props.productName,
            .deviceName = props.deviceName,
    };
}

void MqttBrokersList::mqttPropsToBrokerList(const MqttProperties& props,  std::vector<BrokerInfo>& result) {
    result.clear();
    for (const auto& prop : props.brokers) {
        result.push_back(brokerInfoFromProperties(prop));
    }
}