//
// Created by Ivan Kishchenko on 31/08/2023.
//

#pragma once

#include "BTConfig.h"
#include "core/Registry.h"

class BTManager : public TService<Service_Lib_BTManager> {
public:
    explicit BTManager(Registry &registry) : TService(registry) {}
    void setup() override;
};
