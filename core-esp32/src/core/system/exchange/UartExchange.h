//
// Created by Ivan Kishchenko on 22/6/24.
//

#pragma once

#include <driver/uart.h>
#include <soc/gpio_num.h>
#include <driver/gpio.h>
#include "core/system/System.h"

#include "ZeroMQ.h"

struct UartExchangeProperties : TProperties<Props_Sys_Uart, System::Sys_Core> {
    uart_port_t port{UART_NUM_0};
    int baudRate{921600};
    gpio_num_t rxPin{GPIO_NUM_20};
    gpio_num_t txPin{GPIO_NUM_21};
};

void fromJson(const cJSON *json, UartExchangeProperties &props);

ESP_EVENT_DECLARE_BASE(UART_EXCHANGE_INTERNAL_EVENT);

enum {
    Uart_Exchange_Rx,
};

class UartExchange : public TService<UartExchange, Service_Sys_UartExchange, Sys_Core>,
                     public ZeroMQTransport,
                     public TPropertiesConsumer<UartExchange, UartExchangeProperties> {
    UartExchangeProperties _props;
    QueueHandle_t _rxQueue{};
    QueueHandle_t _txQueue{};

    enum Mode {
        Master,
        Slave
    };
    Mode _mode{Master};

    std::function<void(const void *data, size_t size)> _onData;

    std::unique_ptr<ZeroMQChannel> _channel;
private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        auto *self = static_cast<UartExchange *>(arg);
        self->eventHandler(event_base, event_id, event_data);
    }

    void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);


    struct UartMessage {
        size_t size{0};
        void *data{nullptr};
    };

    static UartMessage makeUartMessage(const void *data, size_t size) {
        return {
                .size = size,
                .data = memcpy(malloc(size), data, size),
        };
    }

    static void freeUartMessage(UartMessage &msg) {
        free(msg.data);
    }

private:
    static void rxTask(void *args) {
        auto *self = static_cast<UartExchange *>(args);
        self->rxTask();
    }

    [[noreturn]] void rxTask();

    static void txTask(void *args) {
        auto *self = static_cast<UartExchange *>(args);
        self->txTask();
    }

    [[noreturn]] void txTask();

public:
    explicit UartExchange(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "uart-ex";
    }

    void setup() override;

    void apply(const UartExchangeProperties &props);

    bool post(const void *data, size_t size, TickType_t delay = portMAX_DELAY);

public:
    void sendData(const void *data, size_t size) override {
        post(data, size);
    }

    void sendString(std::string_view data) override {
        sendData(data.data(), data.size());
    }

    void sendUint8(uint8_t msg) override {
        sendData(&msg, sizeof(uint16_t));
    }

    void sendUint16(uint16_t msg) override {
        sendData(&msg, sizeof(uint16_t));
    }

    void sendUint32(uint32_t msg) override {
        sendData(&msg, sizeof(uint32_t));
    }

    void sendUint64(uint64_t msg) override {
        sendData(&msg, sizeof(uint64_t));
    }

    virtual void onData(std::function<void(const void *data, size_t size)> callback) {
        _onData = callback;
    }

    virtual void onError(std::function<void(uint32_t errCode)>) {

    }
};
