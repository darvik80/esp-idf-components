//
// Created by Ivan Kishchenko on 12/10/2023.
//

#include "WifiProperties.h"

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
