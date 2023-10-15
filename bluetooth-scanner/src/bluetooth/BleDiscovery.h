//
// Created by Ivan Kishchenko on 15/10/2023.
//

#pragma once

#include "BTConfig.h"

class BleDiscovery : public TService<BleDiscovery, Service_Lib_BleDiscovery, SysLib_BT>,
                     public TEventSubscriber<BleDiscovery, BleDiscoveryRequest> {
public:
    explicit BleDiscovery(Registry &registry) : TService(registry) {}

    void setup() override;

    void onEvent(const BleDiscoveryRequest& msg);
};
