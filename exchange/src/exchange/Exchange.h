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
#include <core/system/SystemEvent.h>
#include <core/system/SystemService.h>
#include <freertos/FreeRTOS.h>

#include "fmt/format.h"
#include "ComponentConfig.h"

#define STX_HDR 0xFDFE

#define STX_BIT 0x7e
#define ETX_BIT 0xef

struct SystemEventChanged;

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

#define DMA_ALIGNMENT_BYTES	4
#define DMA_ALIGNMENT_MASK	(DMA_ALIGNMENT_BYTES-1)
#define IS_DMA_ALIGNED(VAL)	(!((VAL)& DMA_ALIGNMENT_MASK))
#define MAKE_DMA_ALIGNED(VAL)  (VAL += DMA_ALIGNMENT_BYTES - \
((VAL)& DMA_ALIGNMENT_MASK))

#pragma pack(push, 1)

typedef struct {
    uint8_t stx;
    uint8_t if_type;
    uint8_t if_num;
    uint8_t flags;
    uint16_t seq_num;
    uint8_t pkt_type;
    uint16_t offset;
    uint16_t payload_len;
    uint16_t checksum;
    uint8_t etx;
} transfer_header_t;

typedef struct {
    uint8_t if_type;
    uint8_t if_num;
    uint8_t flags;
    uint16_t seq_num;
    uint8_t pkt_type;
    uint16_t length;

    union {
        transfer_header_t *hdr;
        void *payload;
    };
} exchange_message_t;

#pragma pack(pop)

class Exchange {
public:
    virtual void send(const exchange_message_t &msg) = 0;

    virtual void onMessage(const exchange_message_t &msg) = 0;

    virtual ~Exchange() = default;
};

class ExchangeDevice {
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _rx_queue{};
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _tx_queue{};

protected:
    ExchangeDevice();

    bool hasData();

    uint16_t computeChecksum(void *buf, uint16_t len);

    QueueHandle_t getRxQueue(ExchangeQueuePriority priority) const;

    QueueHandle_t getTxQueue(ExchangeQueuePriority priority) const;

    virtual esp_err_t unpackBuffer(void *buf, exchange_message_t &msg);

    virtual esp_err_t packBuffer(const exchange_message_t &origin, exchange_message_t &msg);

    virtual esp_err_t postRxBuffer(exchange_message_t &rxBuf) const;

    virtual esp_err_t getNextTxBuffer(exchange_message_t &txBuf);

public:
    virtual esp_err_t writeData(const exchange_message_t &buffer, TickType_t tick = portMAX_DELAY);

    virtual esp_err_t readData(exchange_message_t &buffer, TickType_t tick = 1);

    virtual ~ExchangeDevice() {
        for (size_t idx = 0; idx < MAX_PRIORITY_QUEUES; idx++) {
            vQueueDelete(_rx_queue[idx]);
            vQueueDelete(_tx_queue[idx]);
        }
    }
};

namespace exchange_detail {
    struct ExchangeInternalMessage : TMessage<ComponentEvtId_Internal, Component_Exchange> {
        uint8_t id;
        exchange_message_t message;
    };
}

struct ExchangeMessage : TMessage<ComponentEvtId_ExchangeMessage, Component_Exchange> {
    Exchange &exchange;
    exchange_message_t message;
};

template<ServiceSubId id>
class AbstractExchange :
        public Exchange,
        public TService<AbstractExchange<id>, id, Component_Exchange>,
        public TMessageSubscriber<AbstractExchange<id>, exchange_detail::ExchangeInternalMessage> {
protected:
    ExchangeDevice *_device{};

public:
    explicit AbstractExchange(Registry &registry)
        : TService<AbstractExchange, id, Component_Exchange>(registry) {
    }

    ExchangeDevice *getDevice() {
        return _device;
    }

    void setup() override {
        FreeRTOSTask::execute(
            [this] {
                esp_logi(exchange, "%s task running", this->getServiceName().data());
                exchange_message_t buf_handle{};

                while (true) {
                    if (ESP_OK != _device->readData(buf_handle)) {
                        esp_logi(exchange, "%s ignore message...", this->getServiceName().data());
                        continue;
                    }

                    onMessage(buf_handle);
                }
            },
            this->getServiceName(), 4096
        );
    }

    void send(const exchange_message_t &msg) override {
        if (_device) {
            _device->writeData(msg);
        }
    }

    void onMessage(const exchange_message_t &msg) override {
        getDefaultEventBus().post(exchange_detail::ExchangeInternalMessage{
            .id = id,
            .message = msg
        });
    }

    void handle(const exchange_detail::ExchangeInternalMessage &msg) {
        // process only internal message from correct exchange: case when esp-now & uart work together
        if (msg.id == id) {
            this->getRegistry().getEventBus().send(ExchangeMessage{
                .exchange = *this,
                .message = msg.message
            });
            free(msg.message.payload);
        }
    }
};

#endif //EXCHANGE_H
