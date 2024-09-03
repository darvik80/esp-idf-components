//
// Created by Ivan Kishchenko on 29/8/24.
//

#ifndef SPIDEVICE_H
#define SPIDEVICE_H

#include <array>
#include <cstdint>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum SpiQueuePriority {
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

struct SpiHeader {
    uint8_t if_type;
    uint8_t if_num;
    uint8_t flags;
    uint16_t seq_num;
    uint8_t pkt_type;
    uint16_t offset;
    uint16_t length;
    uint16_t payload_len;
}
        __packed;

struct SpiMessage : SpiHeader {
    void *payload;
}
        __packed;

#define SPI_DMA_ALIGNMENT_BYTES	4
#define SPI_DMA_ALIGNMENT_MASK	(SPI_DMA_ALIGNMENT_BYTES-1)
#define IS_SPI_DMA_ALIGNED(VAL)	(!((VAL)& SPI_DMA_ALIGNMENT_MASK))
#define MAKE_SPI_DMA_ALIGNED(VAL)  (VAL += SPI_DMA_ALIGNMENT_BYTES - \
((VAL)& SPI_DMA_ALIGNMENT_MASK))

class SpiDevice {
    TaskHandle_t _task{nullptr};
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _rx_queue{};
    std::array<QueueHandle_t, MAX_PRIORITY_QUEUES> _tx_queue{};

private:
    static void run(void *args) {
        auto self = static_cast<SpiDevice *>(args);
        self->run();
    }

protected:
    void unpackBuffer(void *payload, SpiMessage &msg);

    QueueHandle_t getRxQueue(SpiQueuePriority prio) const {
        return _rx_queue[prio];
    }

    QueueHandle_t getTxQueue(SpiQueuePriority prio) const {
        return _tx_queue[prio];
    }

    virtual void run() = 0;

public:
    SpiDevice();

    virtual esp_err_t setup();

    virtual void destroy();

    virtual esp_err_t writeData(const SpiMessage *buffer) = 0;

    virtual esp_err_t readData(SpiMessage *buffer) = 0;

    virtual ~SpiDevice();
};


#define PIN_NUM_MISO  (gpio_num_t)5
#define PIN_NUM_MOSI  (gpio_num_t)6
#define PIN_NUM_CLK  (gpio_num_t)4
#define PIN_NUM_CS (gpio_num_t)7

// TODO: move it to properties file
#define PIN_NUM_MASTER_HANDSHAKE (gpio_num_t)8
#define PIN_NUM_MASTER_DATA_READY (gpio_num_t)9

#define PIN_NUM_SLAVE_HANDSHAKE (gpio_num_t)8
#define PIN_NUM_SLAVE_DATA_READY (gpio_num_t)9

// TODO: move config to properties file
#define CONFIG_ESP_SPI_CHECKSUM 1

// #define PRIO_Q_HIGH                     0
// #define PRIO_Q_MID                      1
// #define PRIO_Q_LOW                      2
// #define MAX_PRIORITY_QUEUES             3
//
// TODO: move config to properties file
#define CONFIG_SPI_RX_QUEUE_SIZE	10
#define CONFIG_SPI_TX_QUEUE_SIZE	10

#define SPI_TASK_DEFAULT_STACK_SIZE	 4096
#define SPI_TASK_DEFAULT_PRIO        22
#define SPI_BITS_PER_WORD			8

#define RX_BUF_SIZE              1600

#endif //SPIDEVICE_H
