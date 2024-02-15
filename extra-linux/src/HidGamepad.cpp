//
// Created by Ivan Kishchenko on 25/01/2024.
//

#include "HidGamepad.h"
#include "StringUtils.h"
#include "HidGamepadProperties.h"

#include <hidapi/hidapi_darwin.h>

HidGamepad::HidGamepad(Registry &registry)
        : TService(registry) {
    registry.getPropsLoader().addReader("gamepad", defaultPropertiesReader<HidGamepadProperties>);
    registry.getPropsLoader().addConsumer(this);
}

void HidGamepad::apply(const HidGamepadProperties &props) {
    _props = props;
}

void HidGamepad::setup() {
    //_task.start();
}

[[noreturn]] void HidGamepad::handle() {
    while (true) {
        doProcess();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void HidGamepad::doProcess() {
    auto device = hid_open(_props.vendorId, _props.productId, nullptr);
    if (device) {
        esp_logi(gamepad, "device opened: %04x:%04x", _props.vendorId, _props.productId);
        while (true) {
            unsigned char buf[64]{};
            int res = hid_read_timeout(device, buf, 64, 10000);
            if (res > 0) {
                if (buf[1] & 0x01) {
                    esp_logi(hid, "report: %d", res);
                    std::string bufStr;
                    for (int idx = 0; idx < res; idx++) {
                        char tmp[16];
                        snprintf(tmp, 16, "%02X ", buf[idx]);
                        bufStr += tmp;
                    }
                    esp_logi(hid, "%d:%s", res, bufStr.c_str());
                }
            } else if (res < 0) {
                const wchar_t *err = hid_error(device);
                esp_loge(hid, "handle error: %s", StringUtils::ws2s(err).c_str());
                break;
            }
        }
        hid_close(device);
    } else {
        const wchar_t *err = hid_error(nullptr);
        esp_loge(
                hid, "can't open device %04x:%04x, %s",
                _props.vendorId, _props.productId,
                StringUtils::ws2s(err).c_str()
        );
    }
}

HidGamepad::~HidGamepad() {
    hid_exit();
}

