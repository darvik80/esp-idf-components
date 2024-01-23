//
// Created by Ivan Kishchenko on 12/10/2023.
//

#pragma once

#include "core/system/System.h"

class NvsStorage : public TService<NvsStorage, Service_Sys_NvsStorage, Sys_Core> {
public:
    explicit NvsStorage(Registry &registry) : TService(registry) {}
};
