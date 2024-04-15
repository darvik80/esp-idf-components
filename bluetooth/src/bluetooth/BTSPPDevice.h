//
// Created by Ivan Kishchenko on 25/08/2023.
//

#pragma once

#ifdef CONFIG_BT_CLASSIC_ENABLED

#include <esp_spp_api.h>
#include "BTConfig.h"

struct SppDeviceInfo {
    std::string bdAddr;
    uint32_t handle;
    std::string cache;
};

class BTSppDevice : public TService<BTSppScanner, Service_Lib_BTSppScanner, SysLib_BT> , public TEventSubscriber<BTSppScanner, BTSppConnRequest, BTSppConnected, BTSppDisconnected, BTSppInput> {
    std::unordered_map<uint32_t, SppDeviceInfo> _devices;
public:
    BTSppDevice() = delete;

    BTSppDevice(const BTSppDevice &) = delete;

    BTSppDevice &operator=(const BTSppDevice &) = delete;

    explicit BTSppDevice(Registry &registry);

    void setup() override;

    void onEvent(const BTSppConnRequest& msg);

    void onEvent(const BTSppConnected& msg);

    void onEvent(const BTSppDisconnected& msg);

    void onEvent(const BTSppInput& msg);

};

#endif