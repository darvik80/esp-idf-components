//
// Created by Ivan Kishchenko on 12/10/2023.
//

#include "MqttProperties.h"

void fromJson(cJSON *json, MqttProperties::BrokerInfo &props) {
    cJSON *item = json->child;
    while (item) {
        if (!strcmp(item->string, "type") && item->type == cJSON_String) {
            props.type = item->valuestring;
        } else if (!strcmp(item->string, "uri") && item->type == cJSON_String) {
            props.uri = item->valuestring;
        } else if (!strcmp(item->string, "username") && item->type == cJSON_String) {
            props.username = item->valuestring;
        } else if (!strcmp(item->string, "password") && item->type == cJSON_String) {
            props.password = item->valuestring;
        } else if (!strcmp(item->string, "ca-cert-file") && item->type == cJSON_String) {
            props.caCert = item->valuestring;
        } else if (!strcmp(item->string, "client-cert") && item->type == cJSON_String) {
            props.clientCert = item->valuestring;
        } else if (!strcmp(item->string, "client-key-file") && item->type == cJSON_String) {
            props.clientKey = item->valuestring;
        } else if (!strcmp(item->string, "product-name") && item->type == cJSON_String) {
            props.productName = item->valuestring;
        } else if (!strcmp(item->string, "device-name") && item->type == cJSON_String) {
            props.deviceName = item->valuestring;
        } else if (!strcmp(item->string, "device-secret") && item->type == cJSON_String) {
            props.deviceSecret = item->valuestring;
        }

        item = item->next;
    }
}

void fromJson(cJSON *json, MqttProperties &props) {
    MqttProperties::BrokerInfo info;
    if (json->type == cJSON_Object) {
        fromJson(json, info);
        props.brokers.push_back(info);
    } else if (json->type == cJSON_Array) {
        cJSON *item = json->child;
        while (item) {
            if (item->type == cJSON_Object) {
                fromJson(item, info);
                props.brokers.push_back(info);
            }
            item = item->next;
        }
    }
}