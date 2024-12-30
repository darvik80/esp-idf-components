//
// Created by Ivan Kishchenko on 21/09/2023.
//

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>

#include "core/system/System.h"
#include "WifiProperties.h"
#include <core/StateMachine.h>

namespace wifi {
    enum Action {
        Wifi_ReqConnect,
        Wifi_ReqScan,
        Wifi_EvtApConnected,
        Wifi_EvtConnected,
        Wifi_EvtDisconnected,
        Wifi_EvtScanDone,
    };

    struct DisconnectedState : State<
            StateEvent<Wifi_ReqConnect>, struct ConnectingState,
            StateEvent<Wifi_ReqScan>, struct ScanningState,
            StateEvent<Wifi_EvtScanDone>, struct ScanCompletedState> {
    };

    struct ConnectingState : State<
            StateEvent<Wifi_ReqScan>, struct ScanningState,
            StateEvent<Wifi_EvtDisconnected>, struct DisconnectedState,
            StateEvent<Wifi_EvtApConnected>, struct ApConnectedState> {
    };

    struct ApConnectedState : State<
            StateEvent<Wifi_ReqScan>, struct ScanningState,
            StateEvent<Wifi_EvtConnected>, struct ConnectedState,
            StateEvent<Wifi_EvtDisconnected>, struct DisconnectedState> {
    };

    struct ConnectedState : State<
            StateEvent<Wifi_ReqScan>, struct ScanningState,
            StateEvent<Wifi_EvtDisconnected>, struct DisconnectedState> {
    };

    struct ScanningState : State<
            StateEvent<Wifi_EvtScanDone>, struct ScanCompletedState> {
    };

    struct ScanCompletedState : State<
            StateEvent<Wifi_ReqConnect>, struct ConnectingState,
            StateEvent<Wifi_ReqScan>, struct ScanningState> {
    };
}

class WifiService
        : public TService<WifiService, Service_Sys_Wifi, Sys_Core>,
          public TPropertiesConsumer<WifiService, WifiProperties>,
          public TMessageSubscriber<WifiService, Command>,
          public StateMachine<WifiService, wifi::DisconnectedState, wifi::ConnectingState, wifi::ApConnectedState, wifi::ConnectedState, wifi::ScanningState, wifi::ScanCompletedState> {
    WifiProperties _props;

    esp_netif_t *_netif{nullptr};
private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        auto *self = static_cast<WifiService *>(arg);
        self->eventHandler(event_base, event_id, event_data);
    }

    void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);

public:
    explicit WifiService(Registry &registry);

    [[nodiscard]] std::string_view getServiceName() const override {
        return "wifi";
    }

    void setup() override;

    void handle(const Command &cmd);

    void apply(const WifiProperties &props);

public:
    void onStateChanged(const TransitionTo<wifi::DisconnectedState> &, const wifi_event_sta_disconnected_t &event);

    void onStateChanged(const TransitionTo<wifi::ConnectingState> &);

    void onStateChanged(const TransitionTo<wifi::ApConnectedState> &, const wifi_event_sta_connected_t &event);

    void onStateChanged(const TransitionTo<wifi::ConnectedState> &, const ip_event_got_ip_t &event);

    void onStateChanged(const TransitionTo<wifi::ScanningState> &);

    void onStateChanged(const TransitionTo<wifi::ScanCompletedState> &);
};

std::string getWifiMacAddress();