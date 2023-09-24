//
// Created by Ivan Kishchenko on 31/08/2023.
//

#include "BTManager.h"

#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_bt.h>

void BTManager::setup()  {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.bt_max_acl_conn=3;
    bt_cfg.bt_max_sync_conn=3;
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_dev_set_device_name("magic-lamp"));
}
