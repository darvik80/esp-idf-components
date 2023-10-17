//
// Created by Ivan Kishchenko on 29/08/2023.
//

#pragma once

#ifdef CONFIG_BT_CLASSIC_ENABLED

#include <esp_gap_bt_api.h>
#include <map>
#include <string>

#include "BTConfig.h"

class BTGapDiscovery : public TService<BTGapDiscovery, Service_Lib_BTGapDiscovery, SysLib_BT>, public TEventSubscriber<BTGapDiscovery, BTGapDiscoveryRequest, BTGapDiscoveryStart, BTGapDeviceInfo, BTGapDiscoveryDone> {
    std::map<std::string, BTGapDeviceInfo> _devices;
public:
    explicit BTGapDiscovery(Registry &registry) : TService(registry) {}

    void onEvent(const BTGapDiscoveryRequest& req);

    void onEvent(const BTGapDiscoveryStart& start);

    void onEvent(const BTGapDeviceInfo& devInfo);

    void onEvent(const BTGapDiscoveryDone& done);

    void setup() override;
};

#endif