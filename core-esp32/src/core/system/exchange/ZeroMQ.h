//
// Created by Ivan Kishchenko on 28/6/24.
//

#pragma once

#include <core/StateMachine.h>
#include <string>
#include <map>
#include <optional>
#include <type_traits>

enum Event {
    ZeroMQ_Disconnected,
    ZeroMQ_Connecting,
    ZeroMQ_Greeting,
    ZeroMQ_Ready,
    ZeroMQ_Connected,
    ZeroMQ_Error,
};

struct ZeroMQDisconnectedState : State<
        StateEvent<ZeroMQ_Connecting>, struct ZeroMQConnectingState> {
};

struct ZeroMQConnectingState : State<
        StateEvent<ZeroMQ_Greeting>, struct ZeroMQGreetingState> {
};

struct ZeroMQGreetingState : State<
        StateEvent<ZeroMQ_Greeting>, struct ZeroMQGreetingState,
        StateEvent<ZeroMQ_Ready>, struct ZeroMQReadyState,
        StateEvent<ZeroMQ_Error>, struct ZeroMQErrorState> {
};

struct ZeroMQReadyState : State<
        StateEvent<ZeroMQ_Connected>, struct ZeroMQConnectedState,
        StateEvent<ZeroMQ_Error>, struct ZeroMQErrorState> {
};

struct ZeroMQConnectedState : State<
        StateEvent<ZeroMQ_Error>, struct ZeroMQErrorState> {
};

struct ZeroMQErrorState : State<
        StateEvent<ZeroMQ_Connected>, struct ZeroMQErrorState,
        StateEvent<ZeroMQ_Disconnected>, struct ZeroMQDisconnectedState> {
};

class ZeroMQTransport {
public:
    virtual void flush() = 0;

    virtual void sendString(std::string_view data) = 0;

    virtual void sendData(const void *data, size_t size) = 0;

    virtual void sendUint8(uint8_t byte) = 0;

    virtual void sendUint16(uint16_t byte) = 0;

    virtual void sendUint32(uint32_t byte) = 0;

    virtual void sendUint64(uint64_t byte) = 0;

    virtual void onData(std::function<void(const void *data, size_t size)>) = 0;

    virtual void onError(std::function<void(uint32_t errCode)>) = 0;
};

struct ZeroMQGreeting {
    std::array<uint8_t, 10> signature{0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F};
    std::array<uint8_t, 2> version{0x03, 0x00};
    std::array<uint8_t, 20> mechanism{
            'N', 'U', 'L', 'L', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t isServer{0x00};
    std::array<uint8_t, 32> filler{};
};

struct ZeroMQCommand {
    std::string_view command;

    explicit ZeroMQCommand(std::string_view command) : command(command) {}

    virtual size_t size() const = 0;
};

struct ZeroMQReady : ZeroMQCommand {
    std::map<std::string, std::string> properties;

    ZeroMQReady() : ZeroMQCommand("READY") {}

    [[nodiscard]] size_t size() const override {
        size_t size = command.size() + 1;
        if (!properties.empty()) {
            for (const auto &property: properties) {
                size += property.first.size() + 1 + property.second.size() + 4;
            }
        }
        return size;
    }
};

struct ZeroMQError : ZeroMQCommand {
    std::string errorReason;

    ZeroMQError() : ZeroMQCommand("ERROR") {}

    [[nodiscard]] size_t size() const override {
        size_t size = command.size() + 1 + errorReason.size();
        return size;
    }
};


struct ZeroMQMessage {
    std::vector<uint8_t> data;

    [[nodiscard]] size_t size() const {
        return data.size();
    }
};

class ZeroMQMessagedHandler {
public:
    virtual void onGreeting(const ZeroMQGreeting &msg) = 0;

    virtual void onReady(const ZeroMQReady &msg) = 0;

    virtual void onError(const ZeroMQError &msg) = 0;

    virtual void onMessage(const ZeroMQMessage &msg) = 0;
};

class NetworkBuffer {
    std::vector<uint8_t> _cache;
public:
    void append(const void *data, size_t size) {
        _cache.insert(_cache.end(), static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + size);
    }

    void consume(size_t size) {
        _cache.erase(_cache.begin(), _cache.begin() + size);
    }

    size_t available() {
        return _cache.size();
    }
};

class ZeroMQCodec {
public:
    void send(ZeroMQTransport &transport, const ZeroMQReady &msg) {
        auto size = msg.size();
        if (size < UINT8_MAX) {
            transport.sendUint8(0x04);
            transport.sendUint8(size);
        } else {
            transport.sendUint8(0x06);
            transport.sendUint32(size);
        }

        transport.sendUint8(5);
        transport.sendString("READY");
        for (auto &property: msg.properties) {
            transport.sendUint8(property.first.size());
            transport.sendData(property.first.data(), property.first.size());
            transport.sendUint32(property.second.size());
            transport.sendData(property.second.data(), property.second.size());
        }
    }

    void send(ZeroMQTransport &transport, const ZeroMQError &msg) {
        auto size = msg.size();
        if (size < UINT8_MAX) {
            transport.sendUint8(0x04);
            transport.sendUint8(size);
        } else {
            transport.sendUint8(0x06);
            transport.sendUint32(size);
        }

        transport.sendUint8(msg.command.size());
        transport.sendString(msg.command.data());
    }

public:
    void send(ZeroMQTransport &transport, const ZeroMQGreeting &msg) {
        transport.sendData(msg.signature.data(), msg.signature.size());
        transport.sendData(msg.version.data(), msg.version.size());
        transport.sendData(msg.mechanism.data(), msg.mechanism.size());
        transport.sendUint8(msg.isServer);
        transport.sendData(msg.filler.data(), msg.filler.size());
    }

    void send(ZeroMQTransport &transport, const ZeroMQCommand &msg) {
        if (msg.command == "READY") {
            send(transport, static_cast<const ZeroMQReady &>(msg));
        } else if (msg.command == "ERROR") {
            send(transport, static_cast<const ZeroMQError &>(msg));
        }
    }

    void send(ZeroMQTransport &transport, const ZeroMQMessage &msg) {
        auto size = msg.size();
        if (size < UINT8_MAX) {
            transport.sendUint8(0x00);
            transport.sendUint8(size);
        } else {
            transport.sendUint8(0x02);
            transport.sendUint32(size);
        }

        transport.sendData(msg.data.data(), msg.data.size());
    }
};

class ZeroMQChannel : public StateMachine<
        ZeroMQChannel,
        ZeroMQDisconnectedState,
        ZeroMQConnectingState,
        ZeroMQGreetingState,
        ZeroMQReadyState,
        ZeroMQConnectedState,
        ZeroMQErrorState> {
private:
    std::vector<uint8_t> _cache;
    ZeroMQCodec _codec;
    ZeroMQTransport &_transport;
    bool _isServer{false};
public:
    explicit ZeroMQChannel(ZeroMQTransport &transport, bool isServer);

    void onConnect();

    void onRecv(const void *data, size_t size);

    void onError(uint32_t errCode);

    void onDisconnect(uint32_t errCode);

public:
    void onStateChanged(const TransitionTo<ZeroMQDisconnectedState> &state, uint32_t errCode);

    void onStateChanged(const TransitionTo<ZeroMQConnectingState> &state);

    void onStateChanged(const TransitionTo<ZeroMQGreetingState> &state);

    void onStateChanged(const TransitionTo<ZeroMQReadyState> &state);

    void onStateChanged(const TransitionTo<ZeroMQConnectedState> &state);

    void onStateChanged(const TransitionTo<ZeroMQErrorState> &state, uint32_t errCode);
};

class Reader {
public:
    virtual uint8_t readUint8() {
        uint8_t val;
        read(&val, sizeof(uint8_t));

        return val;
    }

    virtual uint16_t readUint16() {
        uint16_t val;
        read(&val, sizeof(uint16_t));

        return val;
    }

    virtual uint32_t readUint32() {
        uint32_t val;
        read(&val, sizeof(uint32_t));

        return val;
    }

    virtual uint64_t readUint64() {
        uint64_t val;
        read(&val, sizeof(uint64_t));

        return val;
    }

    virtual size_t read(void *buf, size_t bufSize) = 0;

    template<typename T>
    size_t read(T& val) {
        return read(&val, sizeof(T));
    }
};

class Writer {
public:
    virtual size_t sendUint8(uint8_t val) {
        return send(&val);
    }

    virtual size_t sendUint16(uint16_t val) {
        return send(&val);
    }

    virtual size_t sendUint32(uint32_t val) {
        return send(&val);
    }

    virtual size_t sendUint64(uint64_t val) {
        return send(&val);
    }

    virtual size_t send(const void *buf, size_t bufSize) = 0;

    template<typename T>
    inline size_t send(const T& val) {
        return send(&val, sizeof(val));
    }
};
