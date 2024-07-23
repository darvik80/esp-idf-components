//
// Created by Ivan Kishchenko on 12/10/2023.
//

#pragma once

#include "core/system/System.h"

class NvsStorage : public TService<NvsStorage, Service_Sys_NvsStorage, Sys_Core> {
    nvs_handle _handle{};
private:
    static int set(int argc, char **argv);
    static int get(int argc, char **argv);
    static NvsStorage *instance;
public:
    explicit NvsStorage(Registry &registry);

    void set(std::string_view key, int8_t value) const;

    void set(std::string_view key, uint8_t value) const;

    void set(std::string_view key, int16_t value) const;

    void set(std::string_view key, uint16_t value) const;

    void set(std::string_view key, std::string_view value) const;

    void set(std::string_view key, int32_t value) const;

    void set(std::string_view key, uint32_t value) const;

    void set(std::string_view key, int64_t value) const;

    void set(std::string_view key, uint64_t value) const;

    std::string getStr(std::string_view key, std::string_view def = "") const;

    int8_t getInt8(std::string_view key, int8_t def = 0) const;

    uint8_t getUint8(std::string_view key, uint8_t def = 0) const;

    int16_t getInt16(std::string_view key, int16_t def = 0) const;

    uint16_t getUint16(std::string_view key, uint16_t def = 0) const;

    int32_t getInt32(std::string_view key, int32_t def = 0) const;

    uint32_t getUint32(std::string_view key, uint32_t def = 0) const;

    int64_t getInt64(std::string_view key, int64_t def = 0) const;

    uint64_t getUint64(std::string_view key, uint64_t def = 0) const;

    void commit() const;

    ~NvsStorage() override;
};
