//
// Created by Ivan Kishchenko on 31/08/2023.
//

#pragma once

#include "BTConfig.h"

#ifdef CONFIG_BT_ENABLED

class BTManager : public TService<BTManager, Service_Lib_BTManager, SysLib_BT> {
public:
    explicit BTManager(Registry &registry) : TService(registry) {}

    void setup() override;
};

#endif
