//
// Created by Ivan Kishchenko on 29/08/2023.
//

#pragma once

#include <sdkconfig.h>

#ifdef CONFIG_BT_ENABLED

#include <string>
#include <esp_bt_defs.h>
#include <esp_gap_bt_api.h>

class BTUtils {
public:
    static const char *bda2str(esp_bd_addr_t bda, char res[18]);
    static std::string bda2str(esp_bd_addr_t bda);
    static const char *bda2str(const uint8_t* bda, char res[18]);
    static bool str2bda(const char* str, esp_bd_addr_t& bda);

    static const char *eir2str(uint8_t *eir, char res[ESP_BT_GAP_EIR_DATA_LEN]);
};

#endif