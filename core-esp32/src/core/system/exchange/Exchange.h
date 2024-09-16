//
// Created by Ivan Kishchenko on 11/9/24.
//

#ifndef EXCHANGE_H
#define EXCHANGE_H

#include <cstdint>

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
    uint8_t if_type;
    uint8_t if_num;
    uint8_t flags;
    uint16_t seq_num;
    uint8_t pkt_type;
    uint16_t offset;
    uint16_t length;
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

#define RX_BUF_SIZE              1600

#endif //EXCHANGE_H
