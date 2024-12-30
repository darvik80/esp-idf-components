//
// Created by darvik on 30.12.2024.
//

#include "SNTPService.h"

#include <core/system/mqtt/MqttService.h>

[[maybe_unused]] void fromJson(const cJSON* json, SNTPSetupMessage& event)
{
    cJSON* item = json->child;
    while (item)
    {
        if (!strcmp(item->string, "time") && item->type == cJSON_String)
        {
            struct tm tm;
            const char* g_s_ios8601_format = "%.4d%.2d%.2dT%.2d%.2d%.2dZ";
            sscanf(
                item->valuestring, g_s_ios8601_format,
                &tm.tm_year,
                &tm.tm_mon,
                &tm.tm_mday,
                &tm.tm_hour,
                &tm.tm_min,
                &tm.tm_sec
            );

            event.time.tv_sec = mktime(&tm);
            event.time.tv_usec = 0;
        }

        item = item->next;
    }
}

SNTPService::SNTPService(Registry& registry): TService(registry)
{
}

void SNTPService::setup()
{
#ifdef CONFIG_LWIP_DHCP_GET_NTP_SRV
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_EXTRA_SERVICE_SNTP_SERVER);
    config.start = true; // start SNTP service explicitly (after connecting)
    config.smooth_sync = true;
    config.server_from_dhcp = true;
    // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
    config.renew_servers_after_new_IP = true;
    // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1; // updates from server num 1, leaving server 0 (from DHCP) intact
    config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
    config.sync_cb = syncNotificationCallback; // only if we need the notification function
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));

    ESP_ERROR_CHECK(esp_netif_sntp_start());
#endif

#ifdef CONFIG_CORE_MQTT_ENABLE
    auto mqtt = getRegistry().getService<MqttService>();
    if (mqtt)
    {
        mqtt->addJsonHandler<SNTPSetupMessage>("/sys/ntp", MQTT_SUB_RELATIVE);
    }
#endif
}

void SNTPService::destroy()
{
#ifdef CONFIG_LWIP_DHCP_GET_NTP_SRV
    esp_netif_sntp_deinit();
#endif
}

void SNTPService::handle(const SNTPSetupMessage& msg)
{
    time_t now = time(nullptr);
    struct tm tm = *gmtime(&now);
    esp_logi(
        sntp, "GMT time: %04d.%.02d.%02d %02d:%02d:%02d",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
    );
}
