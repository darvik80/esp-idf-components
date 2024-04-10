//
// Created by Ivan Kishchenko on 05/03/2024.
//

#include "Beacon.h"

#ifdef CONFIG_BLE_BEACON_ENABLE

#include <cmath>
#include <bluetooth/BTUtils.h>

#define RSSI_N 2

const uint8_t Beacon::S_BEACON_UUID[ESP_UUID_LEN_128] = {
        0xfd, 0xa5, 0x06, 0x93,
        0xa4, 0xe2, 0x4f, 0xb1,
        0xaf, 0xcf, 0xc6, 0xeb,
        0x07, 0x64, 0x78, 0x25
};

Beacon::Beacon(Registry &registry) : TService(registry) {
}

void Beacon::setup() {
    getDefaultEventBus().post(BleDiscoveryRequest{
            .duration = 60,
    });
    _timer.attach(60000, true, [] {
        getDefaultEventBus().post(BleDiscoveryRequest{
                .duration = 60,
        });
    });
    _report.attach(2000, true, [] {
        getDefaultEventBus().post(BleScanDone{});
    });
}


void Beacon::handle(const BleScanResult &msg) {
    if (msg.serviceData.size() == sizeof(BeaconServiceData)) {
        auto sdata = reinterpret_cast<const BeaconServiceData *>(msg.serviceData.data());
        esp_logi(beacon, "uuid: 0x%04x, battery: %d, model: %d", sdata->uuid, sdata->battery, sdata->productModel);
    }
    auto mdata = reinterpret_cast<const BeaconManufacturerType *>(msg.manufacturerType.data());

    auto it = _filter.find(msg.bda);
    if (it == _filter.end()) {
        it = _filter.emplace(msg.bda, KalmanFilter<1, 1>()).first;
        it->second.zeros();
    }
    it->second.predict();
    float measurement[1] = { (float)msg.rssi };
    auto correct = it->second.correct(measurement);
    int rssi = correct ? (int)correct[0] : msg.rssi;
    int rssi1m = mdata->rssi1m;

    auto dist = std::pow(10.0f, (static_cast<double>((int) rssi1m - rssi)) / (10 * RSSI_N));
    esp_logi(beacon, "%s:%s, major: 0x%04x, minor: 0x%04x, rssi1m: %d, rssi: %d:%d, dist: %f", msg.bda.c_str(),
             msg.name.c_str(), mdata->major, mdata->minor, rssi1m, msg.rssi, rssi, dist);
            esp_log_buffer_hex("uuid:", mdata->uuid128, ESP_UUID_LEN_128);
    _beacons[msg.bda] = BeaconInfo{
            .bda = msg.bda,
            .name = msg.name,
            .rssi = msg.rssi,
            .rssif = rssi,
            .rssi1m = rssi1m,
            .distance = dist
    };
}

void Beacon::handle(const BleScanDone &msg) {
    uint8_t addrType{};
    esp_bd_addr_t bda{};
    esp_ble_gap_get_local_used_addr(bda, &addrType);
    LocationTagReport report;
    report.bda = BTUtils::bda2str(bda);
    for (const auto &item: _beacons) {
        report.beacons.push_back(item.second);
        esp_logw(beacon, "%s:%s, distance: %f", item.first.c_str(), item.second.name.c_str(), item.second.distance);
    }
    if (!report.beacons.empty()) {
        getDefaultEventBus().send(report);
    }

    //_beacons.clear();
}

const BleDiscoveryFilter Beacon::getFilter() {
    return BleDiscoveryFilter{
            .manufacturerSpecificTypeFilter = [](const uint8_t *ptr, size_t len) {
                if (ptr && len == sizeof(BeaconManufacturerType)) {
                    auto mdata = reinterpret_cast<const BeaconManufacturerType *>(ptr);
                    if (0 == memcmp(mdata->uuid128, S_BEACON_UUID, ESP_UUID_LEN_128)) {
                        return true;
                    }
                }

                return false;
            }
    };
}

#endif
