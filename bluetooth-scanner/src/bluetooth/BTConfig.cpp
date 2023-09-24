//
// Created by Ivan Kishchenko on 03/09/2023.
//
#include "BTConfig.h"

void toJson(cJSON* json, const BTScanner& msg) {
    cJSON_AddStringToObject(json, "bd-addr", msg.bdAddr.c_str());
    cJSON_AddStringToObject(json, "barcode", msg.barcode.c_str());
}