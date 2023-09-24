//
// Created by Ivan Kishchenko on 29/08/2023.
//

#pragma once

#include <cstdint>
#include <esp_hidh.h>
#include <esp_bt_defs.h>

#include "BTConfig.h"
#include "core/Registry.h"

union HidModifiers {
    struct {
        bool rMeta: 1;
        bool rShift: 1;
        bool rCtrl: 1;
        bool lMeta: 1;
        bool lAlt: 1;
        bool lShift: 1;
        bool lCtrl: 1;
    } bits;
    uint8_t all;
};

struct HidKeyboard {
    HidModifiers modifiers;

    uint8_t reserved;
    uint8_t key1;
    uint8_t key2;
    uint8_t key3;
    uint8_t key4;
    uint8_t key5;
    uint8_t key6;
};

struct HidGeneric {
    HidModifiers modifiers;
    uint8_t val;
};

struct HidDeviceInfo {
    std::string bdAddr;
    esp_hidh_dev_t* dev{};
    std::string cache{};
};

class BTHidScanner : public TService<Service_Lib_BTHidScanner> , public TEventSubscriber<BTHidScanner, BTHidConnRequest, BTHidConnected, BTHidDisconnected, BTHidInput> {
    std::unordered_map<std::string, HidDeviceInfo> _devices;
public:
    explicit BTHidScanner(Registry &registry);
    void setup() override;

    void onEvent(const BTHidConnRequest& msg);

    void onEvent(const BTHidConnected& msg);

    void onEvent(const BTHidDisconnected& msg);

    void onEvent(const BTHidInput& msg);
};
