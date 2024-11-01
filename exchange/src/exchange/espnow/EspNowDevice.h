//
// Created by Ivan Kishchenko on 4/10/24.
//

#pragma once

#include <sdkconfig.h>
#ifdef CONFIG_EXCHANGE_BUS_ESP_NOW

#include <esp_now.h>
#include <exchange/Exchange.h>

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} esp_now_send_cb_event_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} esp_now_recv_cb_event_t;

class EspNowDevice : public ExchangeDevice {
    QueueHandle_t _rxQueue;
    SemaphoreHandle_t _txSemaphore;

    FreeRTOSTask _rxTask;
    FreeRTOSTask _txTask;
private:
    static void txCallback(const uint8_t *mac_addr, esp_now_send_status_t status);

    static void rxCallback(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

    void rxTask();

    void txTask();
protected:
    esp_err_t packBuffer(const exchange_message_t &origin, exchange_message_t &msg);
public:
    EspNowDevice();
    ~EspNowDevice();
};

#endif