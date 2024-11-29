//
// Created by Ivan Kishchenko on 4/10/24.
//

#include "EspNowDevice.h"
#ifdef CONFIG_EXCHANGE_BUS_ESP_NOW

#include "EspNowExchange.h"

#include <esp_now.h>
#include <esp_wifi.h>

#define CONFIG_ESPNOW_WIFI_MODE_STATION 1

#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
constexpr wifi_interface_t ESPNOW_WIFI_IF = WIFI_IF_STA;
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
constexpr wifi_interface_t ESPNOW_WIFI_IF = WIFI_IF_AP;
#endif

#define ESPNOW_MAXDELAY 512

static uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

inline std::string get_mac_address(uint8_t mac_addr[ESP_NOW_ETH_ALEN]) {
    char mac_addr_str[19];
    snprintf(mac_addr_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);

    return mac_addr_str;
}

void EspNowDevice::txCallback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    esp_logd(esp_now, "txCallback: %d", status);
    if (mac_addr == nullptr) {
        esp_logw(esp_now, "Send cb mac_addr is empty");
        return;
    }

    auto *espNow = Registry::getInstance().getService<EspNowExchange>();
    auto *device = static_cast<EspNowDevice *>(espNow->getDevice());
    xSemaphoreGive(device->_txSemaphore);
}

void EspNowDevice::rxCallback(const esp_now_recv_info_t *info, const uint8_t *data, int data_len) {
    esp_logd(
        esp_now, "rxCallback: %d, %s - %s", data_len,
        get_mac_address(info->src_addr).c_str(),
        get_mac_address(info->des_addr).c_str()
    );

    esp_now_recv_cb_event_t event{
        .data = (uint8_t *) malloc(data_len),
        .data_len = data_len
    };
    memcpy(event.mac_addr, info->src_addr, ESP_NOW_ETH_ALEN);
    memcpy(event.data, data, data_len);

    auto *espNow = Registry::getInstance().getService<EspNowExchange>();
    auto *device = static_cast<EspNowDevice *>(espNow->getDevice());
    if (xQueueSend(device->_rxQueue, &event, ESPNOW_MAXDELAY) != pdTRUE) {
        esp_logi(esp_now, "send espNow event failed");
    }
}

EspNowDevice::EspNowDevice() {

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t peer{};
    peer.channel = 0;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;

    // if wifi module not init - init and repeat get_mac
    if (ESP_ERR_WIFI_NOT_INIT == esp_wifi_get_mac(ESPNOW_WIFI_IF, peer.peer_addr)) {
        ESP_ERROR_CHECK(esp_netif_init());
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
        ESP_ERROR_CHECK(esp_wifi_get_mac(ESPNOW_WIFI_IF, peer.peer_addr));
    }

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    ESP_ERROR_CHECK(esp_now_register_send_cb(txCallback));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(rxCallback));

    // /* Set primary master key. */
    // ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = 0;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;
    memcpy(peer.peer_addr, s_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    _rxTask = FreeRTOSTask::submit([this]() {
        rxTask();
    }, "esp-now-rx", 4096);

    _txTask = FreeRTOSTask::submit([this]() {
        txTask();
    }, "esp-now-rx", 4096);

    _rxQueue = xQueueCreate(CONFIG_EXCHANGE_BUS_RX_QUEUE_SIZE, sizeof(esp_now_recv_info_t));
    _txSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(_txSemaphore);
}

EspNowDevice::~EspNowDevice() {
    _rxTask.shutdown();
    _txTask.shutdown();
    vQueueDelete(_rxQueue);
    vSemaphoreDelete(_txSemaphore);
}

esp_err_t EspNowDevice::packBuffer(const exchange_message_t &origin, exchange_message_t &msg) {
    constexpr uint16_t offset = sizeof(transfer_header_t);
    uint16_t total_len = origin.length + offset;

    if (total_len > CONFIG_EXCHANGE_ESP_NOW_BUS_BUFFER) {
        esp_loge(device, "Max frame length exceeded %d.. drop it", total_len);
        return ESP_FAIL;
    }

    msg.if_type = origin.if_type;
    msg.if_num = origin.if_num;
    msg.flags = origin.flags;
    msg.seq_num = origin.seq_num;
    msg.pkt_type = origin.pkt_type;
    msg.length = total_len;
    msg.payload = malloc(total_len);
    assert(msg.payload);
    memset (msg.payload, 0, total_len);

    auto *hdr = static_cast<transfer_header_t *>(msg.payload);
    hdr->stx = STX_BIT;
    hdr->if_type = origin.if_type;
    hdr->if_num = origin.if_num;
    hdr->flags = origin.flags;
    hdr->seq_num = origin.seq_num;
    hdr->pkt_type = origin.pkt_type;
    hdr->checksum = 0;
    hdr->offset = offset;
    hdr->payload_len = origin.length;
    hdr->etx = ETX_BIT;
    /* copy the data from caller */
    if (origin.length) {
        memcpy(msg.payload + offset, origin.payload, origin.length);
    }
    hdr->checksum = computeChecksum(msg.payload, hdr->payload_len);

    return ESP_OK;
}

void EspNowDevice::rxTask() {
    esp_logi(esp_now, "txTask running");
    esp_now_recv_cb_event_t event;
    while (xQueueReceive(_rxQueue, &event, portMAX_DELAY)) {
        exchange_message_t rx_buf{};
        if (ESP_OK != unpackBuffer(event.data, rx_buf)) {
            free(event.data);
            esp_logw(esp_now, "drop msg, can't unpack message");
        } else if (ESP_OK != postRxBuffer(rx_buf)) {
            free(event.data);
            esp_logw(esp_now, "ignore msg, can't post message");
        }
    }
}

void EspNowDevice::txTask() {
    esp_logi(esp_now, "txTask running");
    while (true) {
        // send data
        exchange_message_t tx_buf{};
        if (ESP_OK == getNextTxBuffer(tx_buf)) {
            xSemaphoreTake(_txSemaphore, portMAX_DELAY);
            auto err = esp_now_send(s_broadcast_mac, (uint8_t *) tx_buf.payload, tx_buf.length);
            if (err != ESP_OK) {
                esp_logw(esp_now, "esp_now_send failed, %s", esp_err_to_name(err));
            }
            free(tx_buf.payload);
        }
    }
}

#endif