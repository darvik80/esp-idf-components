//
// Created by Ivan Kishchenko on 08/08/2023.
//

#include "Logger.h"
#include "Properties.h"

[[maybe_unused]] void fromJson(cJSON *json, WifiProperties::AccessPoint &props) {
    cJSON *item = json->child;
    while (item) {
        if (!strcmp(item->string, "ssid") && item->type == cJSON_String) {
            props.ssid = item->valuestring;
        } else if (!strcmp(item->string, "password") && item->type == cJSON_String) {
            props.password = item->valuestring;
        }

        item = item->next;
    }
}

[[maybe_unused]] void fromJson(cJSON *json, WifiProperties &props) {
    WifiProperties::AccessPoint ap;
    if (json->type == cJSON_Object) {
        fromJson(json, ap);
        props.ap.push_back(ap);
    } else if (json->type == cJSON_Array) {
        cJSON *item = json->child;
        while (item) {
            if (item->type == cJSON_Object) {
                fromJson(item, ap);
                props.ap.push_back(ap);
            }
            item = item->next;
        }
    }
}

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

void PropertiesLoader::load(std::string_view filePath) {
    std::string cfg;
    char buf[64];
    if (auto *file = fopen(filePath.data(), "r"); file) {
        esp_logi(props, "read props file: %s", filePath.data());
        while (fgets(buf, 64, file) != nullptr) {
            cfg.append(buf);
        }
        fclose(file);

        cJSON *json = cJSON_Parse(cfg.c_str());
        cJSON *item = json->child;
        while (item) {
            esp_logi(props, "read props: %s", item->string);
            if (item->type == cJSON_Object || item->type == cJSON_Array) {
                if (
                    auto it = _readers.find(item->string);it != _readers.end()) {
                    auto props = it->second(item);
                    for (
                        auto consumer: _consumers) {
                        consumer->applyProperties(*props);
                    }
                }
            }
            item = item->next;
        }
        cJSON_Delete(json);
    } else {
        esp_logi(props, "can't read props: %s", filePath.data());
    }
}