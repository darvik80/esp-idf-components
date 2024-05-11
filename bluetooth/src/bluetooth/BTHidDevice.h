//
// Created by Ivan Kishchenko on 29/08/2023.
//

#pragma once

#ifdef CONFIG_BT_ENABLED
#include <cstdint>
#include <esp_hidh.h>
#include <esp_bt_defs.h>

#include "BTConfig.h"

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

struct HidGamePad {
    uint8_t leftAxisX;
    uint8_t leftAxisY;
    uint8_t rightAxisX;
    uint8_t rightAxisY;
    uint8_t cursor;
    union {
        uint8_t all;
        struct {
            bool a: 1;     // 0x01
            bool b: 1;     // 0x02
            bool _1: 1;    // 0x04
            bool x: 1;     // 0x08
            bool y: 1;     // 0x10
            bool _2: 1;    // 0x20
            bool lb: 1;    // 0x40
            bool rb: 1;    // 0x80
        };
    } keys1;
    union {
        uint8_t all;
        struct {
            bool lt: 1;         //0x01
            bool rt: 1;         //0x02
            bool select: 1;     //0x04
            bool start: 1;      //0x08
            bool _1: 1;         // 0x10
            bool leftAxis: 1;   //0x20
            bool rightAxis: 1;  //0x40
        };
    } keys2;
    uint8_t rt;
    uint8_t lt;
    uint16_t reserved;
};

struct HidDeviceInfo {
    std::string bdAddr;
    esp_hidh_dev_t *dev{};
    std::string cache{};
};

class BTHidDevice
        : public TService<BTHidDevice, Service_Lib_BTHidScanner, SysLib_BT>,
          public TMessageSubscriber<BTHidDevice, BTHidConnRequest, BTHidConnected, BTHidDisconnected, BTHidInput> {
    std::unordered_map<std::string, HidDeviceInfo> _devices;
public:
    explicit BTHidDevice(Registry &registry);

    void setup() override;

    void handle(const BTHidConnRequest &msg);

    void handle(const BTHidConnected &msg);

    void handle(const BTHidDisconnected &msg);

    void handle(const BTHidInput &msg);
};

#endif