//
// Created by Ivan Kishchenko on 25/08/2023.
//

#pragma once

#include <esp_spp_api.h>
#include "BTConfig.h"
#include "core/Registry.h"

struct SppDeviceInfo {
    std::string bdAddr;
    uint32_t handle;
    std::string cache;
};

class BTSppScanner : public TService<Service_Lib_BTSppScanner> , public TEventSubscriber<BTSppScanner, BTSppConnRequest, BTSppConnected, BTSppDisconnected, BTSppInput> {
    std::unordered_map<uint32_t, SppDeviceInfo> _devices;
public:
    explicit BTSppScanner(Registry &registry);

    void setup() override;

    void onEvent(const BTSppConnRequest& msg);

    void onEvent(const BTSppConnected& msg);

    void onEvent(const BTSppDisconnected& msg);

    void onEvent(const BTSppInput& msg);

};
