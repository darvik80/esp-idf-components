//
// Created by Ivan Kishchenko on 28/6/24.
//

#include <cstring>
#include "ZeroMQ.h"

ZeroMQChannel::ZeroMQChannel(ZeroMQTransport& transport, bool isServer) :_transport(transport), _isServer(isServer) {

}

void ZeroMQChannel::onConnect() {
    StateMachine::handle(StateEvent<ZeroMQ_Greeting>{});
}

void ZeroMQChannel::onRecv(const void *data, size_t size) {
    _cache.insert(_cache.end(), (const char*)data, (const char*)data + size);
    if (std::get_if<ZeroMQConnectingState *>(&getCurrentState())) {
        if (_cache.size() >= sizeof(ZeroMQGreeting)) {
            ZeroMQGreeting msg;
            memcpy(&msg, _cache.data(), sizeof(msg));
            _cache.erase(_cache.begin(), _cache.begin() + sizeof(msg));
            if (msg.signature[0] != 0xFF || msg.signature[9] != 0x7F) {
                _transport.flush();
            } else {
                StateMachine::handle(StateEvent<ZeroMQ_Greeting>{});
            }
        }
    } else if (std::get_if<ZeroMQGreetingState *>(&getCurrentState())) {

    }
}

void ZeroMQChannel::onError(uint32_t errCode) {

}

void ZeroMQChannel::onDisconnect(uint32_t errCode) {

}

void ZeroMQChannel::onStateChanged(const TransitionTo<ZeroMQDisconnectedState> &state, uint32_t errCode) {

}

void ZeroMQChannel::onStateChanged(const TransitionTo<ZeroMQConnectingState> &state) {
    if (_isServer) {
        ZeroMQGreeting msg;
        msg.isServer = true;
        _transport.sendData(&msg, sizeof(msg));
    }
}

void ZeroMQChannel::onStateChanged(const TransitionTo<ZeroMQGreetingState> &state) {
    if (!_isServer) {
        ZeroMQGreeting msg;
        msg.isServer = false;
        _transport.sendData(&msg, sizeof(msg));
    } else {
        ZeroMQReady ready;
        ready.properties["Socket-Type"] = "UART";
        _codec.send(_transport, ready);
        StateMachine::handle(StateEvent<ZeroMQ_Ready>{});
    }
}

void ZeroMQChannel::onStateChanged(const TransitionTo<ZeroMQReadyState> &state) {
    StateMachine::handle(StateEvent<ZeroMQ_Connected>{});
}

void ZeroMQChannel::onStateChanged(const TransitionTo<ZeroMQConnectedState> &state) {

}

void ZeroMQChannel::onStateChanged(const TransitionTo<ZeroMQErrorState> &state, uint32_t errCode) {

}
