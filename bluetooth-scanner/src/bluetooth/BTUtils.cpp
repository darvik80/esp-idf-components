//
// Created by Ivan Kishchenko on 29/08/2023.
//

#include "BTUtils.h"

#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_bt.h>
#include <esp_gap_bt_api.h>
#include <memory.h>

const char *BTUtils::bda2str(esp_bd_addr_t bda, char res[18]) {
    if (bda == nullptr) {
        return nullptr;
    }

    uint8_t *p = bda;
    sprintf(res, ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(p));

    return res;
}

const char *BTUtils::bda2str(const uint8_t *bda, char res[18]) {
    if (bda == nullptr) {
        return nullptr;
    }

    sprintf(res, ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));

    return res;
}

bool BTUtils::str2bda(const char* str, esp_bd_addr_t& res) {
    if (str == nullptr || strlen(str) != 17) {
        return false;
    }

    uint8_t *p = res;
    sscanf(str, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]);

    return true;
}

const char *BTUtils::eir2str(uint8_t *eir, char res[ESP_BT_GAP_EIR_DATA_LEN]) {
    uint8_t rmt_bdname_len = 0;
    if (!eir) {
        return nullptr;
    }

    uint8_t *rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname) {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname) {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (res) {
            memcpy(res, rmt_bdname, rmt_bdname_len);
            res[rmt_bdname_len] = '\0';
        }
    }

    return res;
}

void BTUtils::destroy() {
    ESP_ERROR_CHECK(esp_bluedroid_disable());
    ESP_ERROR_CHECK(esp_bluedroid_deinit());
}
