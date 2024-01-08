//
// Created by Ivan Kishchenko on 03/09/2023.
//


#pragma once

#include <esp_gap_bt_api.h>
#include <esp_hidh.h>
#include <cJSON.h>

#include <string>

#include <core/Core.h>

enum LibraryService {
    SysLib_BT = 0x03,
};

enum BTServiceId {
    Service_Lib_BTManager,
    Service_Lib_BTGapDiscovery,
    Service_Lib_BTSppScanner,
    Service_Lib_BleDiscovery,
    Service_Lib_BTHidScanner,
};

enum BTMessageId {
    BT_MsgId_DiscoveryRequest,
    BT_MsgId_DiscoveryStart,
    BT_MsgId_DiscoveryDevInfo,
    BT_MsgId_DiscoveryDone,

    BT_MsgId_BleDiscoveryRequest,
    BT_MsgId_BleDiscoveryStart,
    BT_MsgId_BleDiscoveryDevInfo,
    BT_MsgId_BleDiscoveryDone,

    BT_MsgId_HidConnRequest,
    BT_MsgId_HidConnected,
    BT_MsgId_HidDisconnected,
    BT_MsgId_HidInput,

    BT_MsgId_SppConnRequest,
    BT_MsgId_SppConnected,
    BT_MsgId_SppDisconnected,
    BT_MsgId_SppInput,

    BT_MsgId_Scanner,

    BT_MsgId_Command,
};

struct BTGapDiscoveryRequest : TEvent<BT_MsgId_BleDiscoveryRequest, SysLib_BT> {

};

struct BTGapDiscoveryStart : TEvent<BT_MsgId_BleDiscoveryStart, SysLib_BT> {

};

struct BTGapDeviceInfo : TEvent<BT_MsgId_DiscoveryDevInfo, SysLib_BT> {
    char bdAddr[18]{0};
    char name[ESP_BT_GAP_MAX_BDNAME_LEN]{0};
    uint32_t cod{0};
    uint8_t rssi{0};
};

struct BTGapDiscoveryDone : TEvent<BT_MsgId_DiscoveryDone, SysLib_BT> {

};

struct BleDiscoveryRequest : TEvent<BT_MsgId_DiscoveryRequest, SysLib_BT> {

};

struct BleDiscoveryStart : TEvent<BT_MsgId_DiscoveryStart, SysLib_BT> {

};


struct BTHidConnRequest : TEvent<BT_MsgId_HidConnRequest, SysLib_BT> {
    char bdAddr[18]{0};
    esp_hid_transport_t transport{ESP_HID_TRANSPORT_BT};
    esp_ble_addr_type_t addrType{BLE_ADDR_TYPE_PUBLIC};
};

struct BTHidConnected : TEvent<BT_MsgId_HidConnected, SysLib_BT> {
    char bdAddr[18]{0};
    esp_hidh_dev_t* dev{};
};

struct BTHidInput : TEvent<BT_MsgId_HidInput, SysLib_BT> {
    char bdAddr[18]{0};
    esp_hidh_dev_t* dev{};
    esp_hid_usage_t usage{};
    char data[10]{};
};

struct BTHidDisconnected : TEvent<BT_MsgId_HidDisconnected, SysLib_BT> {
    char bdAddr[18]{0};
    esp_hidh_dev_t* dev{};
};

struct BTSppConnRequest : TEvent<BT_MsgId_SppConnRequest, SysLib_BT> {
    char bdAddr[18]{0};
};

struct BTSppConnected : TEvent<BT_MsgId_SppConnected, SysLib_BT> {
    char bdAddr[18]{0};
    uint32_t handle;
};

struct BTSppInput : TEvent<BT_MsgId_SppInput, SysLib_BT> {
    uint32_t handle{0};
    char data[32]{0};
};

struct BTSppDisconnected : TEvent<BT_MsgId_SppDisconnected, SysLib_BT> {
    uint32_t handle;
};

struct BTScanner : public TEvent<BT_MsgId_Scanner, SysLib_BT> {
    char bdAddr[18]{};
    char barcode[64]{};
};

inline BTScanner make(std::string_view bdAddr, std::string barcode) {
    BTScanner msg;
    strncpy(msg.bdAddr, bdAddr.data(), sizeof(msg.bdAddr));
    strncpy(msg.barcode, barcode.data(), sizeof(msg.barcode));

    return msg;
}

void toJson(cJSON* json, const BTScanner& msg);

enum BTSubCommand {
    BT_SubId_Scan,
};

template<BTSubCommand subCmd>
struct BTCommand : TEvent<BT_MsgId_Command, SysLib_BT, subCmd> {

};

struct BTScanCommand : BTCommand<BT_SubId_Scan> {

};