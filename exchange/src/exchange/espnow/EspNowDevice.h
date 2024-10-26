//
// Created by Ivan Kishchenko on 4/10/24.
//

#pragma once

#include <exchange/Exchange.h>


class EspNowDevice : public ExchangeDevice {
protected:
    void exchange();
public:
    EspNowDevice();
};
