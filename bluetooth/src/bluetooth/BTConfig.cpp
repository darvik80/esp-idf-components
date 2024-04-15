//
// Created by Ivan Kishchenko on 03/09/2023.
//
#include "BTConfig.h"

#ifdef CONFIG_BT_ENABLED

void toJson(cJSON* json, const BTScanner& msg) {
    cJSON_AddStringToObject(json, "bd-addr", msg.bdAddr);
    cJSON_AddStringToObject(json, "barcode", msg.barcode);
}

#endif