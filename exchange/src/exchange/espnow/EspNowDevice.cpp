//
// Created by Ivan Kishchenko on 4/10/24.
//

#include "EspNowDevice.h"

#include <esp_now.h>

#include <esp_now.h>
#include <esp_wifi.h>

#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
constexpr wifi_interface_t ESPNOW_WIFI_IF = WIFI_IF_STA;
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
constexpr wifi_interface_t ESPNOW_WIFI_IF = WIFI_IF_AP;
#endif

void espNowSendCallback(const uint8_t *mac_addr, esp_now_send_status_t status) {
}

void espNowRecvCallback(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
}

void EspNowDevice::exchange() {
}

EspNowDevice::EspNowDevice() {
    ESP_ERROR_CHECK(esp_now_init());

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t peer{};
    peer.channel = 0;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;

    esp_wifi_get_mac(ESPNOW_WIFI_IF, peer.peer_addr);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    ESP_ERROR_CHECK(esp_now_register_send_cb(espNowSendCallback));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espNowRecvCallback));
}
