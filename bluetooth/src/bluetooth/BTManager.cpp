//
// Created by Ivan Kishchenko on 31/08/2023.
//

#include "BTManager.h"

#ifdef CONFIG_BT_ENABLED

#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_bt.h>
#include <esp_console.h>
#include <argtable3/argtable3.h>

struct bt_args_t {
    struct arg_str *subCmd;
    struct arg_str *bda;
    struct arg_end *end;
};

static bt_args_t bt_args;

#ifdef CONFIG_BT_CLASSIC_ENABLED
static int btCmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bt_args);

    if (nerrors != 0) {
        arg_print_errors(stderr, bt_args.end, argv[0]);
        return 1;
    }

    if (bt_args.subCmd->count == 1) {
        if (strcmp(bt_args.subCmd->sval[0], "scan") == 0) {
            getDefaultEventBus().post(BTGapDiscoveryRequest{});
            printf("ok\r\n");
        }else if (strcmp(bt_args.subCmd->sval[0], "conn") == 0) {
            BTHidConnRequest cmd{
                    .transport = ESP_HID_TRANSPORT_BT,
            };
            strncpy(cmd.bdAddr, bt_args.bda->sval[0], std::min((size_t) 18, strlen(bt_args.bda->sval[0])));
            getDefaultEventBus().post(cmd);
            printf("try open: %s\r\n", bt_args.bda->sval[0]);
        }
    }

    return 0;
}

#endif

static int bleCmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &bt_args);

    if (nerrors != 0) {
        arg_print_errors(stderr, bt_args.end, argv[0]);
        return 1;
    }

    if (bt_args.subCmd->count == 1) {
        if (strcmp(bt_args.subCmd->sval[0], "scan") == 0) {
            getDefaultEventBus().post(BleDiscoveryRequest{});
            printf("ok\r\n");
        } else if (strcmp(bt_args.subCmd->sval[0], "conn") == 0) {
            BTHidConnRequest cmd{
                .transport = ESP_HID_TRANSPORT_BLE,
            };
            strncpy(cmd.bdAddr, bt_args.bda->sval[0], std::min((size_t) 18, strlen(bt_args.bda->sval[0])));
            getDefaultEventBus().post(cmd);
            printf("try open: %s\r\n", bt_args.bda->sval[0]);
        }
    }

    return 0;
}

static void registerBlueTouch() {
    bt_args.subCmd = arg_str1(nullptr, nullptr, "<operation>", "scan, connect, ...");
    bt_args.bda = arg_strn(nullptr, "bda", "xx:xx:xx:xx:xx:xx", 0, 1, "bluedroid address");
    bt_args.end = arg_end(3);

#ifdef CONFIG_BT_CLASSIC_ENABLED
    const esp_console_cmd_t cmdBt = {
            .command = "bt",
            .help = "bluetooth operations",
            .hint = "<opearion> --bda={}",
            .func = &btCmd,
            .argtable = &bt_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmdBt));
#endif

#ifdef CONFIG_BT_BLE_ENABLED
    const esp_console_cmd_t cmdBle = {
            .command = "ble",
            .help = "ble operations",
            .hint = "<opearion> --bda={}",
            .func = &bleCmd,
            .argtable = &bt_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmdBle));
#endif
}

void BTManager::setup() {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    registerBlueTouch();
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    esp_bluedroid_config_t cfg {
        .ssp_en = true
    };
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_dev_set_device_name("robot-espidf"));

#ifdef CONFIG_BT_CLASSIC_ENABLED
    esp_bt_cod_t cod;
    cod.major = ESP_BT_COD_MAJOR_DEV_TOY;
    esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_MAJOR_MINOR);
#endif
}


#endif