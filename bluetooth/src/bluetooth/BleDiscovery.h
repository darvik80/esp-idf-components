//
// Created by Ivan Kishchenko on 15/10/2023.
//

#pragma once

#include "BTConfig.h"

#ifdef CONFIG_BT_BLE_ENABLED

#include <esp_gap_ble_api.h>

struct BleDiscoveryFilter {
    std::function<bool(const uint8_t* ptr, size_t len)> nameFilter = [](const uint8_t* ptr, size_t len) { return true; };
    std::function<bool(const uint8_t* ptr, size_t len)> serviceDataFilter = [](const uint8_t* ptr, size_t len) { return true; };
    std::function<bool(const uint8_t* ptr, size_t len)> manufacturerSpecificTypeFilter = [](const uint8_t* ptr, size_t len) { return true; };
};

class BleDiscovery : public TService<BleDiscovery, Service_Lib_BleDiscovery, SysLib_BT>,
                     public TMessageSubscriber<BleDiscovery, BleDiscoveryRequest> {
    BleDiscoveryFilter _filter;

    static std::weak_ptr<BleDiscovery> s_self;
private:
    static void gapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
public:
    explicit BleDiscovery(Registry &registry, const BleDiscoveryFilter & filter) : TService(registry), _filter{filter} {}

    void setup() override;

    void handle(const BleDiscoveryRequest& msg);


};

#endif
