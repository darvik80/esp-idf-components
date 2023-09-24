//
// Created by Ivan Kishchenko on 29/08/2023.
//

#include "BTGapDiscovery.h"

#include <core/Logger.h>
#include <esp_bt.h>
#include "BTUtils.h"
#include "core/Helpers.h"

void gapCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_DISC_RES_EVT: {
            BTGapDeviceInfo deviceInfo;
            BTUtils::bda2str(param->disc_res.bda, deviceInfo.bdAddr);
            esp_logi(gap, "Services for device %s found", deviceInfo.bdAddr);
            for (int idx = 0; idx < param->disc_res.num_prop; idx++) {
                switch (param->disc_res.prop[idx].type) {
                    case ESP_BT_GAP_DEV_PROP_BDNAME:
                        strncpy((char*)deviceInfo.name, (const char*)param->disc_res.prop[idx].val, param->disc_res.prop[idx].len);
                        break;
                    case ESP_BT_GAP_DEV_PROP_EIR:
                        BTUtils::eir2str((uint8_t *) param->disc_res.prop[idx].val, deviceInfo.name);
                        break;
                    case ESP_BT_GAP_DEV_PROP_COD:
                        deviceInfo.cod = *(uint32_t *) (param->disc_res.prop[idx].val);
                        break;
                    case ESP_BT_GAP_DEV_PROP_RSSI:
                        deviceInfo.rssi = *(int8_t *) (param->disc_res.prop[idx].val);
                        break;
                }
            }

            getDefaultEventBus().post(deviceInfo);
            break;
        }
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                esp_logi(gap, "Device discovery stopped.");
                getDefaultEventBus().post(BTGapDiscoveryDone{});
            } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
                esp_logi(gap, "Discovery started.");
                getDefaultEventBus().post(BTGapDiscoveryStart{});
            }
            break;
        }
        case ESP_BT_GAP_MODE_CHG_EVT:
            esp_logi(gap, "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
            break;
        case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
            esp_logi(gap, "ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT");
            break;
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            esp_logi(gap, "ESP_BT_GAP_AUTH_CMPL_EVT");
            break;
        default: {
            esp_logi(gap, "gap event: %d", event);
            break;
        }
    }
}

void BTGapDiscovery::setup() {
    getRegistry().getEventBus().subscribe(this);
    /* set discoverable and connectable mode, wait to be connected */
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));

    /* register GAP callback function */
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gapCallback));
}

void BTGapDiscovery::onEvent(const BTGapDiscoveryRequest &req) {
    esp_logi(gap, "request scan");
    ESP_ERROR_CHECK(esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0));
}

void BTGapDiscovery::onEvent(const BTGapDiscoveryStart &start) {
    _devices.clear();
}

void BTGapDiscovery::onEvent(const BTGapDeviceInfo &devInfo) {
    if (auto it = _devices.find(devInfo.bdAddr);it != _devices.end()) {
        if (!strlen(it->second.name)) {
            strcpy(it->second.name, devInfo.name);
        }
    } else {
        _devices.emplace((const char *) devInfo.bdAddr,  devInfo);
    }
}

void BTGapDiscovery::onEvent(const BTGapDiscoveryDone &done) {
    for (auto &device: _devices) {
        esp_logi(gap, "Device: %s", device.second.bdAddr);
        esp_logi(gap, "\tname: %s", device.second.name);
        esp_logi(gap, "\t cod: %lu", device.second.cod);
        esp_logi(gap, "\trssi: %d", device.second.rssi);
    }
}
