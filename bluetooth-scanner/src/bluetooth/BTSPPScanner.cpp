//
// Created by Ivan Kishchenko on 25/08/2023.
//

#include "BTSPPScanner.h"
#include "BTUtils.h"
#include "BTManager.h"

#include "core/Helpers.h"
#include "core/Logger.h"

#include <esp_bt_main.h>
#include <esp_bt.h>
#include <esp_spp_api.h>

static esp_bd_addr_t lastBda;

static void sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            if (param->init.status == ESP_SPP_SUCCESS) {
                esp_logi(spp, "spp init success");
            } else {
                esp_logi(spp, "spp init failed: %d", param->init.status);
            }
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            esp_logi(spp, "ESP_SPP_DISCOVERY_COMP_EVT scn_num:%d, status: %d", param->disc_comp.scn_num,
                     param->disc_comp.status);
            if (param->disc_comp.status == ESP_SPP_SUCCESS) {
                for (int i = 0; i < param->disc_comp.scn_num; i++) {
                    esp_logi(spp, "-- [%d] scn: %d service_name: %s", i, param->disc_comp.scn[i],
                             param->disc_comp.service_name[i]);

                    ESP_ERROR_CHECK(
                            esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER, param->disc_comp.scn_num, lastBda));
                }
            }
            break;
        case ESP_SPP_START_EVT:
            esp_logi(spp, "ESP_SPP_START_EVT scn: %d, status: %d", param->start.scn, param->start.status);
            break;
        case ESP_SPP_OPEN_EVT:
            if (param->open.status == ESP_SPP_SUCCESS) {

                BTSppConnected msg;
                BTUtils::bda2str(param->open.rem_bda, msg.bdAddr);
                esp_logi(spp, "ESP_SPP_OPEN_EVT handle:%" PRIu32 " rem_bda:[%s]", param->open.handle, msg.bdAddr);

                msg.handle = param->open.handle;
                getDefaultEventBus().post(msg);
            } else {
                esp_logi(spp, "ESP_SPP_OPEN_EVT status:%d", param->open.status);
            }
            break;
        case ESP_SPP_CLOSE_EVT: {
            esp_logi(spp, "ESP_SPP_CLOSE_EVT status:%d handle:%" PRIu32 " close_by_remote:%d", param->close.status,
                     param->close.handle, param->close.async);
            BTSppDisconnected msg;
            msg.handle = param->close.handle;
            getDefaultEventBus().post(msg);
        }
            break;
        case ESP_SPP_DATA_IND_EVT: {
            BTSppInput msg;
            msg.handle = param->data_ind.handle;
            memcpy(msg.data, param->data_ind.data, std::min((uint16_t) 32, param->data_ind.len));
            getDefaultEventBus().post(msg);
        }
            break;
        case ESP_SPP_CL_INIT_EVT:
            if (param->cl_init.status == ESP_SPP_SUCCESS) {
                esp_logi(spp, "ESP_SPP_CL_INIT_EVT handle:%" PRIu32 " sec_id:%d", param->cl_init.handle,
                         param->cl_init.sec_id);
            } else {
                esp_loge(spp, "ESP_SPP_CL_INIT_EVT status:%d", param->cl_init.status);
            }
            break;
        default:
            esp_logi(spp, "spp event: %d", event);
            break;
    }
}

BTSppScanner::BTSppScanner(Registry &registry) : TService(registry) {
    registry.getEventBus().subscribe(this);
}

void BTSppScanner::setup() {
    ESP_ERROR_CHECK(esp_spp_register_callback(sppCallback));

    esp_spp_cfg_t bt_spp_cfg = {
            .mode = ESP_SPP_MODE_CB,
            .enable_l2cap_ertm = true,
            .tx_buffer_size = 0,
    };
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&bt_spp_cfg));
}

void BTSppScanner::onEvent(const BTSppConnRequest &msg) {
    esp_logi(spp, "dev:[%s] req conn", msg.bdAddr.c_str());
    BTUtils::str2bda(msg.bdAddr.c_str(), lastBda);

    ESP_ERROR_CHECK(esp_spp_start_discovery(lastBda));
}

void BTSppScanner::onEvent(const BTSppConnected &msg) {
    esp_logi(spp, "dev:[%s] connected", msg.bdAddr);
    _devices.emplace(msg.handle, SppDeviceInfo{
            .bdAddr = msg.bdAddr,
            .handle = msg.handle,
            .cache = {}
    });
}

void BTSppScanner::onEvent(const BTSppDisconnected &msg) {
    if (auto it = _devices.find(msg.handle); it != _devices.end()) {
        esp_logi(spp, "dev:[%s] disconnected", it->second.bdAddr.c_str());
        _devices.erase(it);
    }
}

void BTSppScanner::onEvent(const BTSppInput &msg) {
    if (auto it = _devices.find(msg.handle); it != _devices.end()) {
        it->second.cache.append(msg.data);
        if (!it->second.cache.empty() && (it->second.cache.ends_with('\n'))) {
            it->second.cache.resize(it->second.cache.size() - 1);
            while (it->second.cache.ends_with('\r') || it->second.cache.ends_with('\n')) {
                it->second.cache.pop_back();
            }
            while (it->second.cache.starts_with(' ') || it->second.cache.starts_with('\t') ||
                   it->second.cache.starts_with('\n') || it->second.cache.starts_with('\n')) {
                it->second.cache.erase(it->second.cache.begin());
            }
            if (!it->second.cache.empty()) {
                esp_logi(spp, "dev:[%s], key: [%s]", it->second.bdAddr.c_str(), it->second.cache.c_str());
                BTScanner barcodeMsg;
                barcodeMsg.bdAddr = it->second.bdAddr;
                barcodeMsg.barcode = it->second.cache;
                getRegistry().getEventBus().send(barcodeMsg);
                it->second.cache.clear();
            }
        }
    }
}
