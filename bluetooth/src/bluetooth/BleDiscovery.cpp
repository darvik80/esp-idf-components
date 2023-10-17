//
// Created by Ivan Kishchenko on 15/10/2023.
//

#include "BleDiscovery.h"

#ifdef CONFIG_BT_BLE_ENABLED

#include <esp_gap_ble_api.h>
#include <esp_hidh.h>
#include <esp_gattc_api.h>


static const char *s_gattc_evt_names[] = {"REG", "UNREG", "OPEN", "READ_CHAR", "WRITE_CHAR", "CLOSE", "SEARCH_CMPL",
                                          "SEARCH_RES", "READ_DESCR", "WRITE_DESCR", "NOTIFY", "PREP_WRITE", "EXEC",
                                          "ACL", "CANCEL_OPEN", "SRVC_CHG", "", "ENC_CMPL_CB", "CFG_MTU", "ADV_DATA",
                                          "MULT_ADV_ENB", "MULT_ADV_UPD", "MULT_ADV_DATA", "MULT_ADV_DIS", "CONGEST",
                                          "BTH_SCAN_ENB", "BTH_SCAN_CFG", "BTH_SCAN_RD", "BTH_SCAN_THR",
                                          "BTH_SCAN_PARAM", "BTH_SCAN_DIS", "SCAN_FLT_CFG", "SCAN_FLT_PARAM",
                                          "SCAN_FLT_STATUS", "ADV_VSC", "", "", "", "REG_FOR_NOTIFY",
                                          "UNREG_FOR_NOTIFY", "CONNECT", "DISCONNECT", "READ_MULTIPLE", "QUEUE_FULL",
                                          "SET_ASSOC", "GET_ADDR_LIST", "DIS_SRVC_CMPL"};

const char *gattc_evt_str(uint8_t event) {
    if (event >= (sizeof(s_gattc_evt_names) / sizeof(*s_gattc_evt_names))) {
        return "UNKNOWN";
    }
    return s_gattc_evt_names[event];
}

static const char *ble_gap_evt_names[] = {"ADV_DATA_SET_COMPLETE", "SCAN_RSP_DATA_SET_COMPLETE",
                                          "SCAN_PARAM_SET_COMPLETE", "SCAN_RESULT", "ADV_DATA_RAW_SET_COMPLETE",
                                          "SCAN_RSP_DATA_RAW_SET_COMPLETE", "ADV_START_COMPLETE", "SCAN_START_COMPLETE",
                                          "AUTH_CMPL", "KEY", "SEC_REQ", "PASSKEY_NOTIF", "PASSKEY_REQ", "OOB_REQ",
                                          "LOCAL_IR", "LOCAL_ER", "NC_REQ", "ADV_STOP_COMPLETE", "SCAN_STOP_COMPLETE",
                                          "SET_STATIC_RAND_ADDR", "UPDATE_CONN_PARAMS", "SET_PKT_LENGTH_COMPLETE",
                                          "SET_LOCAL_PRIVACY_COMPLETE", "REMOVE_BOND_DEV_COMPLETE",
                                          "CLEAR_BOND_DEV_COMPLETE", "GET_BOND_DEV_COMPLETE", "READ_RSSI_COMPLETE",
                                          "UPDATE_WHITELIST_COMPLETE"};

const char *ble_gap_evt_str(uint8_t event) {
    if (event >= (sizeof(ble_gap_evt_names)/sizeof(*ble_gap_evt_names))) {
        return "UNKNOWN";
    }
    return ble_gap_evt_names[event];
}

static const char *ble_addr_type_names[] = {"PUBLIC", "RANDOM", "RPA_PUBLIC", "RPA_RANDOM"};

const char *ble_addr_type_str(esp_ble_addr_type_t ble_addr_type)
{
    if (ble_addr_type > BLE_ADDR_TYPE_RPA_RANDOM) {
        return "UNKNOWN";
    }
    return ble_addr_type_names[ble_addr_type];
}

#define GAP_DBG_PRINTF(...) printf(__VA_ARGS__)

static void handle_ble_device_result(esp_ble_gap_cb_param_t::ble_scan_result_evt_param *scan_rst)
{
    uint16_t uuid = 0;
    uint16_t appearance = 0;
    char name[64] = {0};

    uint8_t uuid_len = 0;
    uint8_t *uuid_d = esp_ble_resolve_adv_data(scan_rst->ble_adv, ESP_BLE_AD_TYPE_16SRV_CMPL, &uuid_len);
    if (uuid_d != NULL && uuid_len) {
        uuid = uuid_d[0] + (uuid_d[1] << 8);
    }

    uint8_t appearance_len = 0;
    uint8_t *appearance_d = esp_ble_resolve_adv_data(scan_rst->ble_adv, ESP_BLE_AD_TYPE_APPEARANCE, &appearance_len);
    if (appearance_d != NULL && appearance_len) {
        appearance = appearance_d[0] + (appearance_d[1] << 8);
    }

    uint8_t adv_name_len = 0;
    uint8_t *adv_name = esp_ble_resolve_adv_data(scan_rst->ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

    if (adv_name == NULL) {
        adv_name = esp_ble_resolve_adv_data(scan_rst->ble_adv, ESP_BLE_AD_TYPE_NAME_SHORT, &adv_name_len);
    }

    if (adv_name != NULL && adv_name_len) {
        memcpy(name, adv_name, adv_name_len);
        name[adv_name_len] = 0;
    }

    GAP_DBG_PRINTF("BLE: " ESP_BD_ADDR_STR ", ", ESP_BD_ADDR_HEX(scan_rst->bda));
    GAP_DBG_PRINTF("RSSI: %d, ", scan_rst->rssi);
    GAP_DBG_PRINTF("UUID: 0x%04x, ", uuid);
    GAP_DBG_PRINTF("APPEARANCE: 0x%04x, ", appearance);
    GAP_DBG_PRINTF("ADDR_TYPE: '%s'", ble_addr_type_str(scan_rst->ble_addr_type));
    if (adv_name_len) {
        GAP_DBG_PRINTF(", NAME: '%s'", name);
    }
    GAP_DBG_PRINTF("\n");

//    if (uuid == ESP_GATT_UUID_HID_SVC) {
//        add_ble_scan_result(scan_rst->bda, scan_rst->ble_addr_type, appearance, adv_name, adv_name_len, scan_rst->rssi);
//    }
}

const char *esp_ble_key_type_str(esp_ble_key_type_t key_type) {
    const char *key_str = nullptr;
    switch (key_type) {
        case ESP_LE_KEY_NONE:
            key_str = "ESP_LE_KEY_NONE";
            break;
        case ESP_LE_KEY_PENC:
            key_str = "ESP_LE_KEY_PENC";
            break;
        case ESP_LE_KEY_PID:
            key_str = "ESP_LE_KEY_PID";
            break;
        case ESP_LE_KEY_PCSRK:
            key_str = "ESP_LE_KEY_PCSRK";
            break;
        case ESP_LE_KEY_PLK:
            key_str = "ESP_LE_KEY_PLK";
            break;
        case ESP_LE_KEY_LLK:
            key_str = "ESP_LE_KEY_LLK";
            break;
        case ESP_LE_KEY_LENC:
            key_str = "ESP_LE_KEY_LENC";
            break;
        case ESP_LE_KEY_LID:
            key_str = "ESP_LE_KEY_LID";
            break;
        case ESP_LE_KEY_LCSRK:
            key_str = "ESP_LE_KEY_LCSRK";
            break;
        default:
            key_str = "INVALID BLE KEY TYPE";
            break;

    }
    return key_str;
}

static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_logi(bleh, "GATTC EVENT %s", gattc_evt_str(event));

    switch (event) {
        /*
         * SCAN
         * */
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            esp_logi(ble, "BLE GAP EVENT SCAN_PARAM_SET_COMPLETE");
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *) param;
            switch (scan_result->scan_rst.search_evt) {
                case ESP_GAP_SEARCH_INQ_RES_EVT: {
                    handle_ble_device_result(&scan_result->scan_rst);
                    break;
                }
                case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                    esp_logi(ble, "BLE GAP EVENT SCAN DONE: %d", scan_result->scan_rst.num_resps);
                    break;
                default:
                    break;
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
            esp_logi(ble, "BLE GAP EVENT SCAN CANCELED");
            break;
        }

            /*
             * ADVERTISEMENT
             * */
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_logi(ble, "BLE GAP ADV_DATA_SET_COMPLETE");
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            esp_logi(ble, "BLE GAP ADV_START_COMPLETE");
            break;

            /*
             * AUTHENTICATION
             * */
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            if (!param->ble_security.auth_cmpl.success) {
                esp_logi(ble, "BLE GAP AUTH ERROR: 0x%x", param->ble_security.auth_cmpl.fail_reason);
            } else {
                esp_logi(ble, "BLE GAP AUTH SUCCESS");
            }
            break;

        case ESP_GAP_BLE_KEY_EVT: //shows the ble key info share with peer device to the user.
            esp_logi(ble, "BLE GAP KEY type = %s", esp_ble_key_type_str(param->ble_security.ble_key.key_type));
            break;

        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: // ESP_IO_CAP_OUT
            // The app will receive this evt when the IO has Output capability and the peer device IO has Input capability.
            // Show the passkey number to the user to input it in the peer device.
            esp_logi(ble, "BLE GAP PASSKEY_NOTIF passkey:%" PRIu32, param->ble_security.key_notif.passkey);
            break;

        case ESP_GAP_BLE_NC_REQ_EVT: // ESP_IO_CAP_IO
            // The app will receive this event when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
            // show the passkey number to the user to confirm it with the number displayed by peer device.
            esp_logi(ble, "BLE GAP NC_REQ passkey:%" PRIu32, param->ble_security.key_notif.passkey);
            esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
            break;

        case ESP_GAP_BLE_PASSKEY_REQ_EVT: // ESP_IO_CAP_IN
            // The app will receive this evt when the IO has Input capability and the peer device IO has Output capability.
            // See the passkey number on the peer device and send it back.
            esp_logi(ble, "BLE GAP PASSKEY_REQ");
            //esp_ble_passkey_reply(param->ble_security.ble_req.bd_addr, true, 1234);
            break;

        case ESP_GAP_BLE_SEC_REQ_EVT:
            esp_logi(ble, "BLE GAP SEC_REQ");
            // Send the positive(true) security response to the peer device to accept the security request.
            // If not accept the security request, should send the security response with negative(false) accept value.
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;

        default:
            esp_logi(ble, "BLE GAP EVENT %s", ble_gap_evt_str(event));
            break;
    }
}

void BleDiscovery::setup() {
    ESP_ERROR_CHECK(esp_ble_gap_set_device_name("robot-espidf"));

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(ble_gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));
}

#define SCAN_DURATION_SECONDS 5

void BleDiscovery::onEvent(const BleDiscoveryRequest &msg) {
    static esp_ble_scan_params_t hid_scan_params = {
            .scan_type              = BLE_SCAN_TYPE_ACTIVE,
            .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
            .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
            .scan_interval          = 0x50,
            .scan_window            = 0x30,
            .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE,
    };

    esp_err_t ret = ESP_OK;
    if ((ret = esp_ble_gap_set_scan_params(&hid_scan_params)) != ESP_OK) {
        esp_logi(ble, "esp_ble_gap_set_scan_params failed: %d", ret);
    } else {
        if ((ret = esp_ble_gap_start_scanning(SCAN_DURATION_SECONDS)) != ESP_OK) {
            esp_logi(ble, "esp_ble_gap_start_scanning failed: %d", ret);
        }
    }
}

#endif