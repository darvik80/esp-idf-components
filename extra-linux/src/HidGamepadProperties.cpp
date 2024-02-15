//
// Created by Ivan Kishchenko on 09/02/2024.
//

#include "HidGamepadProperties.h"

void fromJson(cJSON *json, HidGamepadProperties &props) {
    HidGamepadProperties info;
    if (json->type == cJSON_Object) {
        cJSON *item = json->child;
        while (item) {
            if (!strcmp(item->string, "vendor-id")) {
                if (item->type == cJSON_Number) {
                    props.vendorId = (uint16_t) item->valuedouble;
                } else if (item->type == cJSON_String) {
                    props.vendorId = (uint16_t)std::stoi(item->valuestring, nullptr, 16);
                }
            } else if (!strcmp(item->string, "product-id")) {
                if (item->type == cJSON_Number) {
                    props.productId = (uint16_t) item->valuedouble;
                } else if (item->type == cJSON_String) {
                    props.productId = (uint16_t)std::stoi(item->valuestring, nullptr, 16);
                }
            }

            item = item->next;
        }
    }
}