//
// Created by Ivan Kishchenko on 4/10/24.
//

#pragma once

#include <sdkconfig.h>

enum ExtraComponentServiceId {
    Service_Extra_SNTP,
};

enum ComponentEventId {
    Service_Extra_EvtId_SNTP,
};

enum ComponentExtra {
    Component_Extra = CONFIG_EXTRA_SERVICES_COMPONENT_ID,
};