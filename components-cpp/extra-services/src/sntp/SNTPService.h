//
// Created by darvik on 30.12.2024.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_LWIP_DHCP_GET_NTP_SRV
#include <esp_netif_sntp.h>
#include <esp_sntp.h>
#endif
#include <core/Registry.h>
#include "ExtraComponentConfig.h"

struct SNTPSetupMessage : TMessage<Service_Extra_EvtId_SNTP, Component_Extra>
{
    timeval time;
};

class SNTPService : public TService<SNTPService, Service_Extra_SNTP, Component_Extra>, public TMessageSubscriber<SNTPService, SNTPSetupMessage>
{
#ifdef CONFIG_LWIP_DHCP_GET_NTP_SRV
    static void syncNotificationCallback(struct timeval* tv)
    {
        sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
        getDefaultEventBus().post(SNTPSetupMessage{
            .time = *tv
        });
    }
#endif

public:
    explicit SNTPService(Registry& registry);

    void setup() override;

    void destroy() override;

    void handle(const SNTPSetupMessage& message);
};
