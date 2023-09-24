//
// Created by Ivan Kishchenko on 29/08/2023.
//

#pragma once


#include <esp_gap_bt_api.h>
#include "core/Registry.h"
#include "BTConfig.h"

#include <map>
#include <string>

class BTGapDiscovery : public TService<Service_Lib_BTGapDiscovery>, public TEventSubscriber<BTGapDiscovery, BTGapDiscoveryRequest, BTGapDiscoveryStart, BTGapDeviceInfo, BTGapDiscoveryDone> {
    std::map<std::string, BTGapDeviceInfo> _devices;
public:
    explicit BTGapDiscovery(Registry &registry) : TService(registry) {}

    void onEvent(const BTGapDiscoveryRequest& req);

    void onEvent(const BTGapDiscoveryStart& start);

    void onEvent(const BTGapDeviceInfo& devInfo);

    void onEvent(const BTGapDiscoveryDone& done);

    void setup() override;
};
