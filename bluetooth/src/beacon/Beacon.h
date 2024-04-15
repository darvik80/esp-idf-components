//
// Created by Ivan Kishchenko on 05/03/2024.
//

#pragma once

#include "bluetooth/BTConfig.h"

#ifdef CONFIG_BLE_BEACON_ENABLE

#include "core/Core.h"
#include "bluetooth/BleDiscovery.h"
#include "core/KalmanFilter.h"

#pragma pack(1)

struct BeaconServiceData {
    uint16_t uuid;
    uint16_t major;
    uint16_t minor;
    uint16_t battery;
    uint8_t productModel;
};


struct BeaconManufacturerType {
    uint8_t id;
    uint8_t length;
    uint8_t uuid128[ESP_UUID_LEN_128];
    uint16_t major;
    uint16_t minor;
    int8_t rssi1m;
};

#pragma pack()

struct BeaconInfo {
    std::string bda;
    std::string name;
    int rssi;
    int rssif;
    int rssi1m;
    double distance;
};

struct LocationTagReport : CMessage<BT_MsgId_BleLocationTagReport, SysLib_BT> {
    std::string bda;
    std::list<BeaconInfo> beacons;

};

inline void toJson(cJSON *json, const BeaconInfo &msg) {
    cJSON_AddStringToObject(json, "bda", msg.bda.c_str());
    if (!msg.name.empty()) {
        cJSON_AddStringToObject(json, "name", msg.name.c_str());
    }
    cJSON_AddNumberToObject(json, "rssi", msg.rssi);
    cJSON_AddNumberToObject(json, "rssif", msg.rssif);
    cJSON_AddNumberToObject(json, "rssi1m", msg.rssi1m);
    cJSON_AddNumberToObject(json, "distance", msg.distance);
}

inline void toJson(cJSON *json, const LocationTagReport &msg) {
    cJSON_AddStringToObject(json, "bda", msg.bda.c_str());
    cJSON *beacons = cJSON_AddArrayToObject(json, "beacons");
    for (const auto &beacon : msg.beacons) {
        cJSON *beaconJson = cJSON_CreateObject();
        toJson(beaconJson, beacon);
        cJSON_AddItemToArray(beacons, beaconJson);
    }
}

class Beacon
        : public TService<Beacon, Service_Lib_BLEBeacon, SysLib_BT>,
          public TMessageSubscriber<Beacon, BleScanResult, BleScanDone> {
    std::unordered_map<std::string, BeaconInfo> _beacons;
    std::map<std::string, KalmanFilter<1,1>> _filter;

    static const uint8_t S_BEACON_UUID[ESP_UUID_LEN_128];

    FreeRTOSTimer _timer;
    FreeRTOSTimer _report;
public:
    explicit Beacon(Registry &registry);

    void setup() override;

    [[nodiscard]] std::string_view getServiceName() const override {
        return "beacon";
    }

    void handle(const BleScanResult &msg);

    void handle(const BleScanDone &msg);

    static const BleDiscoveryFilter getFilter();
};

#endif
