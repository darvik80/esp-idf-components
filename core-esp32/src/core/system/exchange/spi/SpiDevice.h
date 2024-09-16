//
// Created by Ivan Kishchenko on 29/8/24.
//

#ifndef SPIDEVICE_H
#define SPIDEVICE_H

#include <array>
#include <cstdint>
#include <cstring>
#include <core/system/exchange/Exchange.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

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
    void unpackBuffer(void *payload, ExchangeMessage &msg);

    TaskHandle_t getTaskHandle() const {
        return _task;
    }

    QueueHandle_t getRxQueue(int prio) const {
        return _rx_queue[prio];
    }

    QueueHandle_t getTxQueue(int prio) const {
        return _tx_queue[prio];
    }

    virtual void run() = 0;

public:
    SpiDevice();

    virtual esp_err_t setup();

    virtual void destroy();

    virtual esp_err_t writeData(const ExchangeMessage *buffer) = 0;

    virtual esp_err_t readData(ExchangeMessage *buffer) = 0;

    virtual ~SpiDevice();
};

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define PIN_NUM_MISO  (gpio_num_t)5
#define PIN_NUM_MOSI  (gpio_num_t)6
#define PIN_NUM_CLK  (gpio_num_t)4
#define PIN_NUM_CS (gpio_num_t)7

// TODO: move it to properties file
#define PIN_NUM_MASTER_HANDSHAKE (gpio_num_t)8
#define PIN_NUM_MASTER_DATA_READY (gpio_num_t)9

#define PIN_NUM_SLAVE_HANDSHAKE (gpio_num_t)8
#define PIN_NUM_SLAVE_DATA_READY (gpio_num_t)9

#elif CONFIG_IDF_TARGET_ESP32S3
#define PIN_NUM_MISO  (gpio_num_t)19
#define PIN_NUM_MOSI  (gpio_num_t)38
#define PIN_NUM_CLK  (gpio_num_t)18
#define PIN_NUM_CS (gpio_num_t)05

// TODO: move it to properties file
#define PIN_NUM_MASTER_HANDSHAKE (gpio_num_t)21
#define PIN_NUM_MASTER_DATA_READY (gpio_num_t)20

#define PIN_NUM_SLAVE_HANDSHAKE (gpio_num_t)21
#define PIN_NUM_SLAVE_DATA_READY (gpio_num_t)20

#endif

// TODO: move config to properties file
#define CONFIG_ESP_SPI_CHECKSUM 1
#define CONFIG_SPI_RX_QUEUE_SIZE	10
#define CONFIG_SPI_TX_QUEUE_SIZE	10

#define SPI_TASK_DEFAULT_STACK_SIZE	 4096
#define SPI_TASK_DEFAULT_PRIO        22
#define SPI_BITS_PER_WORD			8

#endif //SPIDEVICE_H
