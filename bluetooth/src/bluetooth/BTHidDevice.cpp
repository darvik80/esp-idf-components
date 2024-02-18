//
// Created by Ivan Kishchenko on 29/08/2023.
//

#include "BTHidDevice.h"

#include <esp_hidh.h>
#include "BTUtils.h"
#include "BTHidCodes.h"
#include <esp_hidh_api.h>

char hidGetMap(HidModifiers modifiers, uint8_t code) {
    if (code >= KEY_A and code <= KEY_Z) {
        uint8_t baseCode = (modifiers.all == 0x02 || modifiers.all == 0x20) ? 'A' : 'a';
        return (char) (baseCode + (code - 0x04));
    } else {
        switch (code) {
            case KEY_1:
                return '1';
            case KEY_2:
                return '2';
            case KEY_3:
                return '3';
            case KEY_4:
                return '4';
            case KEY_5:
                return '5';
            case KEY_6:
                return '6';
            case KEY_7:
                return '7';
            case KEY_8:
                return '8';
            case KEY_9:
                return '9';
            case KEY_0:
                return '0';
            case KEY_NONE:
                return 0x00;
            case KEY_ENTER:
                return '\n';
            default:
                break;
        };
    }

    return ' ';
}

void hidhCallback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    auto event = (esp_hidh_event_t) id;
    auto *param = (esp_hidh_event_data_t *) event_data;
    static char lastInput[24];

    switch (event) {
        case ESP_HIDH_START_EVENT: {
            esp_logi(hid, "ESP_HIDH_START_EVENT");
        }
            break;
        case ESP_HIDH_OPEN_EVENT: {
            if (param->open.status == ESP_OK) {
                auto bda = esp_hidh_dev_bda_get(param->open.dev);
                esp_logi(hid, "dev: [" ESP_BD_ADDR_STR "] open: %s", ESP_BD_ADDR_HEX(bda),
                         esp_hidh_dev_name_get(param->open.dev));
                esp_hidh_dev_dump(param->open.dev, stdout);

                BTHidConnected msg;
                BTUtils::bda2str(bda, msg.bdAddr);
                msg.dev = param->open.dev;
                getDefaultEventBus().post(msg);
            } else {
                auto bda = esp_hidh_dev_bda_get(param->open.dev);
                esp_logw(hid, "dev: [" ESP_BD_ADDR_STR "] open failed: %d", ESP_BD_ADDR_HEX(bda), param->open.status);
            }
            break;
        }
        case ESP_HIDH_BATTERY_EVENT: {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->battery.dev);
            esp_logi(hid, "dev: [" ESP_BD_ADDR_STR "] battery: %d%%", ESP_BD_ADDR_HEX(bda), param->battery.level);
            break;
        }
        case ESP_HIDH_INPUT_EVENT: {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
            BTHidInput msg{
                    .usage = param->input.usage,
            };
            BTUtils::bda2str(bda, msg.bdAddr);

            if (ESP_HID_USAGE_KEYBOARD == param->input.usage) {
                if (param->input.length == sizeof(HidKeyboard)) {
                    esp_logi(hid, "dev: [" ESP_BD_ADDR_STR "] input: keyboard", ESP_BD_ADDR_HEX(bda));
                    auto *keyEvent = (HidKeyboard *) param->input.data;
                    msg.data[0] = hidGetMap(keyEvent->modifiers, keyEvent->key1);
                    msg.data[1] = hidGetMap(keyEvent->modifiers, keyEvent->key2);
                    msg.data[2] = hidGetMap(keyEvent->modifiers, keyEvent->key3);
                    msg.data[3] = hidGetMap(keyEvent->modifiers, keyEvent->key4);
                    msg.data[4] = hidGetMap(keyEvent->modifiers, keyEvent->key5);
                    msg.data[5] = hidGetMap(keyEvent->modifiers, keyEvent->key6);

                    getDefaultEventBus().post(msg);
                }
            } else if (ESP_HID_USAGE_GENERIC == param->input.usage) {
                if (param->input.length == sizeof(HidGeneric)) {
                    esp_logi(hid, "dev: [" ESP_BD_ADDR_STR "] input: generic", ESP_BD_ADDR_HEX(bda));

                    auto *keyEvent = (HidGeneric *) param->input.data;
                    msg.data[0] = hidGetMap(keyEvent->modifiers, keyEvent->val);
                    getDefaultEventBus().post(msg);
                }
            } else if (ESP_HID_USAGE_GAMEPAD == param->input.usage) {
                if (param->input.length == 10) {
                    if (memcmp(lastInput, param->input.data, param->input.length) != 0) {
                        esp_logd(
                                hid, "dev: [" ESP_BD_ADDR_STR "] input: gamepad, report: %d, mapIdx: %d",
                                ESP_BD_ADDR_HEX(bda),
                                (int) param->input.report_id,
                                (int) param->input.map_index
                        );
                        memcpy(lastInput, param->input.data, param->input.length);
                        memcpy(msg.data, param->input.data, param->input.length);
                        getDefaultEventBus().post(msg);
                    }
                }
            }

            break;
        }
        case ESP_HIDH_FEATURE_EVENT: {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->feature.dev);
            esp_logi(hid, ESP_BD_ADDR_STR " FEATURE: %8s, MAP: %2u, ID: %3u, Len: %d", ESP_BD_ADDR_HEX(bda),
                     esp_hid_usage_str(param->feature.usage), param->feature.map_index, param->feature.report_id,
                     param->feature.length);
            ESP_LOG_BUFFER_HEX("hid", param->feature.data, param->feature.length);
            break;
        }
        case ESP_HIDH_CLOSE_EVENT: {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->close.dev);
            esp_logi(hid, ESP_BD_ADDR_STR " CLOSE", ESP_BD_ADDR_HEX(bda));
            BTHidDisconnected msg;
            BTUtils::bda2str(bda, msg.bdAddr);
            msg.dev = param->close.dev;
            getDefaultEventBus().post(msg);
            break;
        }
        default:
            esp_logi(hid, "EVENT: %d", event);
            break;
    }
}

BTHidDevice::BTHidDevice(Registry &registry) : TService(registry) {
}

void BTHidDevice::setup() {
    esp_hidh_config_t config = {
            .callback = hidhCallback,
            .event_stack_size = 4096,
            .callback_arg = this,
    };
    ESP_ERROR_CHECK(esp_hidh_init(&config));
}

void BTHidDevice::handle(const BTHidConnRequest &msg) {
    esp_logi(hid, "dev: [%s] req conn", msg.bdAddr);
    esp_bd_addr_t bda;
    BTUtils::str2bda(msg.bdAddr, bda);
    esp_hidh_dev_open(bda, msg.transport, msg.addrType);
}

void BTHidDevice::handle(const BTHidConnected &msg) {
    esp_logi(hid, "dev: [%s] connected", msg.bdAddr);

    _devices.emplace(msg.bdAddr, HidDeviceInfo{
            .bdAddr = msg.bdAddr,
            .dev = msg.dev
    });
}

void BTHidDevice::handle(const BTHidDisconnected &msg) {
    esp_logi(hid, "dev: [%s] disconnected", msg.bdAddr);
    if (auto it = _devices.find(msg.bdAddr); it != _devices.end()) {
        esp_hidh_dev_free(it->second.dev);
        _devices.erase(msg.bdAddr);
    }
}

void BTHidDevice::handle(const BTHidInput &msg) {
    if (auto it = _devices.find(msg.bdAddr); it != _devices.end()) {
        if (msg.usage == ESP_HID_USAGE_GENERIC || msg.usage == ESP_HID_USAGE_KEYBOARD) {
            it->second.cache.append(msg.data);
            if (!it->second.cache.empty() && (it->second.cache.ends_with('\n'))) {
                it->second.cache.resize(it->second.cache.size() - 1);
                while (it->second.cache.starts_with(' ') || it->second.cache.starts_with('\t') ||
                       it->second.cache.starts_with('\n') || it->second.cache.starts_with('\n')) {
                    it->second.cache.erase(it->second.cache.begin());
                }
                if (!it->second.cache.empty()) {
                    esp_logi(hid, "dev:%s, key: [%s]", msg.bdAddr, it->second.cache.c_str());

                    //getRegistry().getEventBus().send(barcodeMsg);
                    getRegistry().getEventBus().post(make(it->second.bdAddr, it->second.cache));
                    it->second.cache.clear();
                }
            }
        }
    }
}
