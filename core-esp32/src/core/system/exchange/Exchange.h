//
// Created by Ivan Kishchenko on 11/9/24.
//

#ifndef EXCHANGE_H
#define EXCHANGE_H

#include <cstdint>
#include <esp_err.h>
#include <esp_timer.h>
#include <core/Registry.h>
#include <core/Task.h>
#include <core/system/SystemService.h>
#include <freertos/FreeRTOS.h>

#include "fmt/format.h"

#define STX_HDR 0xFDFE

enum ExchangeQueuePriority {
    PRIO_Q_HIGH,
    PRIO_Q_MID,
    PRIO_Q_LOW,
    MAX_PRIORITY_QUEUES,
};

enum ESP_INTERFACE_TYPE {
    ESP_STA_IF,
    ESP_AP_IF,
    ESP_HCI_IF,
    ESP_INTERNAL_IF,
    ESP_TEST_IF,
    ESP_MAX_IF,
};

enum ESP_PACKET_TYPE {
    PACKET_TYPE_DATA,
    PACKET_TYPE_COMMAND_REQUEST,
    PACKET_TYPE_COMMAND_RESPONSE,
    PACKET_TYPE_EVENT,
    PACKET_TYPE_EAPOL,
};

struct ExchangeHeader {
    uint16_t stx;
    uint8_t if_type;
    uint8_t if_num;
    uint8_t flags;
    uint16_t seq_num;
    uint8_t pkt_type;
    uint16_t offset;
    uint16_t length;
    uint16_t checksum;
    uint16_t payload_len;
} __packed;

#define DMA_ALIGNMENT_BYTES	4
#define DMA_ALIGNMENT_MASK	(DMA_ALIGNMENT_BYTES-1)
#define IS_DMA_ALIGNED(VAL)	(!((VAL)& DMA_ALIGNMENT_MASK))
#define MAKE_DMA_ALIGNED(VAL)  (VAL += DMA_ALIGNMENT_BYTES - \
((VAL)& DMA_ALIGNMENT_MASK))

struct ExchangeMessage : ExchangeHeader {
    void *payload;
} __packed;


class Exchange {
public:
    virtual void send(const ExchangeMessage &msg) = 0;

    virtual void onMessage(const ExchangeMessage &msg) = 0;

    virtual ~Exchange() = default;
};

class ExchangeDevice {
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _rx_queue{};
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _tx_queue{};

protected:
    ExchangeDevice();

    uint16_t computeChecksum(void *buf, uint16_t len);

    QueueHandle_t getRxQueue(ExchangeQueuePriority priority) const;

    QueueHandle_t getTxQueue(ExchangeQueuePriority priority) const;

    virtual esp_err_t unpackBuffer(void *payload, ExchangeMessage &msg);

    virtual esp_err_t packBuffer(const ExchangeMessage &origin, ExchangeMessage &msg, bool dma);

    virtual esp_err_t postRxBuffer(ExchangeMessage &rxBuf) const;

    virtual esp_err_t getNextTxBuffer(ExchangeMessage &txBuf);

public:
    virtual esp_err_t writeData(const ExchangeMessage &buffer, TickType_t tick = portMAX_DELAY);

    virtual esp_err_t readData(ExchangeMessage &buffer, TickType_t tick = 1);

    virtual ~ExchangeDevice() {
        for (size_t idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
            vQueueDelete(_rx_queue[idx]);
            vQueueDelete(_tx_queue[idx]);
        }
    }
};

template<ServiceSubId id>
class AbstractExchange : public Exchange, public TService<AbstractExchange<id>, id, Sys_Core> {
protected:
    ExchangeDevice *_device{};

public:
    explicit AbstractExchange(Registry &registry)
        : TService<AbstractExchange, id, Sys_Core>(registry) {
    }

    void setup() override {
        FreeRTOSTask::execute(
            [this] {
                esp_logi(exchange, "exchange task running");
                ExchangeMessage buf_handle{};

                while (true) {
                    if (ESP_OK != _device->readData(buf_handle)) {
                        esp_logi(exchange, "ignore message...");
                        continue;
                    }

                    onMessage(buf_handle);
                    free(buf_handle.payload);
                }
            },
            this->getServiceName(), 4096
        );

        FreeRTOSTask::execute([this]() {
            vTaskDelay(pdMS_TO_TICKS(10000));
            while (true) {
                std::string ping = "ping " + std::to_string(esp_timer_get_time());
                ExchangeMessage buf{
                    ExchangeHeader{
                        .if_type = ESP_INTERNAL_IF,
                        .if_num = 0x08,
                        .pkt_type = PACKET_TYPE_COMMAND_REQUEST,
                    },
                };
                buf.payload = (void *) ping.data();
                buf.payload_len = ping.size() + 1;
                buf.length = buf.payload_len;
                esp_logi(exchange, "Send: %s", ping.c_str());
                _device->writeData(buf);
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
        }, "tester", 4096);
    }

    void send(const ExchangeMessage &msg) override {
        if (_device) {
            _device->writeData(msg);
        }
    }

    void onMessage(const ExchangeMessage &msg) override {
        std::string_view ping((char *) (msg.payload + msg.offset), msg.length);
        esp_logi(exchange, "Recv: %s", ping.data());
        if (ping.starts_with("ping")) {
            std::string pong = "pong";
            pong += ping.substr(4);
            ExchangeMessage buf{
                ExchangeHeader{
                    .if_type = ESP_INTERNAL_IF,
                    .if_num = 0x02,
                    .pkt_type = PACKET_TYPE_COMMAND_RESPONSE,
                },
            };
            buf.payload = (void *) pong.data();
            buf.payload_len = pong.size() + 1;
            buf.length = buf.payload_len;

            esp_logi(exchange, "Send: %s", pong.c_str());
            _device->writeData(buf);
        }
    }
};

#endif //EXCHANGE_H
