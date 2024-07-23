//
// Created by Ivan Kishchenko on 12/10/2023.
//

#include <argtable3/argtable3.h>
#include <esp_console.h>
#include "NvsStorage.h"


typedef struct {
    struct arg_str *key;
    struct arg_str *valStr;
    struct arg_int *valI8;
    struct arg_int *valI16;
    struct arg_int *valI32;
    struct arg_int *valI64;
    struct arg_end *end;
} nvs_set_args_t;

static nvs_set_args_t nvs_set_args;

int NvsStorage::set(int argc, char **argv) {
    int errors = arg_parse(argc, argv, (void **) &nvs_set_args);

    if (errors != 0) {
        arg_print_errors(stderr, nvs_set_args.end, argv[0]);
        return 1;
    }

    if (!nvs_set_args.key->count) {
        printf("\tkey & are required\n");
        return 2;
    }

    if (nvs_set_args.valStr->count > 0) {
        NvsStorage::instance->set(nvs_set_args.key->sval[0], nvs_set_args.valStr->sval[0]);
    } else if (nvs_set_args.valI8->count > 0) {
        NvsStorage::instance->set(nvs_set_args.key->sval[0], static_cast<int8_t>(nvs_set_args.valI8->ival[0]));
    } else if (nvs_set_args.valI16->count > 0) {
        NvsStorage::instance->set(nvs_set_args.key->sval[0], static_cast<int16_t>(nvs_set_args.valI16->ival[0]));
    } else if (nvs_set_args.valI32->count > 0) {
        NvsStorage::instance->set(nvs_set_args.key->sval[0], static_cast<int32_t>(nvs_set_args.valI32->ival[0]));
    } else if (nvs_set_args.valI64->count > 0) {
        NvsStorage::instance->set(nvs_set_args.key->sval[0], static_cast<int64_t>(nvs_set_args.valI64->ival[0]));
    }

    NvsStorage::instance->commit();
    printf("\t%s set, ok\n", nvs_set_args.key->sval[0]);

    return 0;
}

typedef struct {
    struct arg_str *key;
    struct arg_str *type;
    struct arg_end *end;
} nvs_get_args_t;

static nvs_get_args_t nvs_get_args;

int NvsStorage::get(int argc, char **argv) {
    int errors = arg_parse(argc, argv, (void **) &nvs_get_args);

    if (errors != 0) {
        arg_print_errors(stderr, nvs_set_args.end, argv[0]);
        return 1;
    }

    if (!nvs_get_args.key->count) {
        printf("\tkey is required\n");
        return 2;
    }

    if (nvs_get_args.type->count) {
        if (!strcmp(nvs_get_args.type->sval[0], "i8")) {
            printf("\ti8: %s:%d\n", nvs_get_args.key->sval[0],
                   NvsStorage::instance->getInt8(nvs_get_args.key->sval[0]));
        } else if (!strcmp(nvs_get_args.type->sval[0], "i16")) {
            printf("\ti16: %s:%d\n", nvs_get_args.key->sval[0],
                   NvsStorage::instance->getInt16(nvs_get_args.key->sval[0]));
        } else if (!strcmp(nvs_get_args.type->sval[0], "i32")) {
            printf("\ti32: %s:%ld\n", nvs_get_args.key->sval[0],
                   NvsStorage::instance->getInt32(nvs_get_args.key->sval[0]));
        } else if (!strcmp(nvs_get_args.type->sval[0], "i64")) {
            printf("\ti64: %s:%lld\n", nvs_get_args.key->sval[0],
                   NvsStorage::instance->getInt64(nvs_get_args.key->sval[0]));
        } else {
            printf("\tstr: %s:%s\n", nvs_get_args.key->sval[0],
                   NvsStorage::instance->getStr(nvs_get_args.key->sval[0], "").c_str());
        }
    } else {
        printf("\tstr: %s:%s\n", nvs_get_args.key->sval[0],
               NvsStorage::instance->getStr(nvs_get_args.key->sval[0], "").c_str());
    }

    return 0;
}

NvsStorage *NvsStorage::instance{nullptr};

NvsStorage::NvsStorage(Registry &registry) : TService(registry) {
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &_handle));

    nvs_set_args.key = arg_str1(nullptr, nullptr, "<key>", nullptr);
    nvs_set_args.valStr = arg_str0("s", "str", "<str val>", nullptr);
    nvs_set_args.valI8 = arg_int0(nullptr, "i8", "<int8 val>", nullptr);
    nvs_set_args.valI16 = arg_int0(nullptr, "i16", "<int16 val>", nullptr);
    nvs_set_args.valI32 = arg_int0("i", "i32", "<int32 val>", nullptr);
    nvs_set_args.valI64 = arg_int0(nullptr, "i64", "<int64 val>", nullptr);
    nvs_set_args.end = arg_end(2);
    esp_console_cmd_t cmd = {
            .command = "nvs-set",
            .help = "nvs set value",
            .hint = nullptr,
            .func = &NvsStorage::set,
            .argtable = &nvs_set_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    nvs_get_args.key = arg_str0(nullptr, nullptr, "<key>", nullptr);
    nvs_get_args.type = arg_str0(nullptr, nullptr, "<type: int8, int16, int32 int64 str>", nullptr);
    nvs_get_args.end = arg_end(1);
    cmd = {
            .command = "nvs-get",
            .help = "nvs get value",
            .hint = nullptr,
            .func = &NvsStorage::get,
            .argtable = &nvs_get_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    instance = this;
}

void NvsStorage::set(std::string_view key, std::string_view value) const {
    ESP_ERROR_CHECK(nvs_set_str(_handle, key.data(), value.data()));
}

void NvsStorage::set(std::string_view key, int8_t value) const {
    ESP_ERROR_CHECK(nvs_set_i8(_handle, key.data(), value));
}

void NvsStorage::set(std::string_view key, uint8_t value) const {
    ESP_ERROR_CHECK(nvs_set_u8(_handle, key.data(), value));
}

void NvsStorage::set(std::string_view key, int16_t value) const {
    ESP_ERROR_CHECK(nvs_set_i16(_handle, key.data(), value));
}

void NvsStorage::set(std::string_view key, uint16_t value) const {
    ESP_ERROR_CHECK(nvs_set_u16(_handle, key.data(), value));
}

void NvsStorage::set(std::string_view key, int32_t value) const {
    ESP_ERROR_CHECK(nvs_set_i32(_handle, key.data(), value));
}

void NvsStorage::set(std::string_view key, uint32_t value) const {
    ESP_ERROR_CHECK(nvs_set_u32(_handle, key.data(), value));
}

void NvsStorage::set(std::string_view key, int64_t value) const {
    ESP_ERROR_CHECK(nvs_set_i64(_handle, key.data(), value));
}

void NvsStorage::set(std::string_view key, uint64_t value) const {
    ESP_ERROR_CHECK(nvs_set_u64(_handle, key.data(), value));
}

std::string NvsStorage::getStr(std::string_view key, std::string_view def) const {
    char buf[64];
    size_t size{sizeof(buf) - 1};
    if (auto err = nvs_get_str(_handle, key.data(), buf, &size); err != ESP_OK) {
        esp_logw(nvs, "nvs_get_str failed: %d", err);
        return def.data();
    }

    return buf;
}

int8_t NvsStorage::getInt8(std::string_view key, int8_t def) const {
    int8_t res{0};
    if (auto err = nvs_get_i8(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

uint8_t NvsStorage::getUint8(std::string_view key, uint8_t def) const {
    uint8_t res{0};
    if (auto err = nvs_get_u8(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

int16_t NvsStorage::getInt16(std::string_view key, int16_t def) const {
    int16_t res{0};
    if (auto err = nvs_get_i16(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

uint16_t NvsStorage::getUint16(std::string_view key, uint16_t def) const {
    uint16_t res{0};
    if (auto err = nvs_get_u16(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

int32_t NvsStorage::getInt32(std::string_view key, int32_t def) const {
    int32_t res{0};
    if (auto err = nvs_get_i32(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

uint32_t NvsStorage::getUint32(std::string_view key, uint32_t def) const {
    uint32_t res{0};
    if (auto err = nvs_get_u32(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

int64_t NvsStorage::getInt64(std::string_view key, int64_t def) const {
    int64_t res{0};
    if (auto err = nvs_get_i64(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

uint64_t NvsStorage::getUint64(std::string_view key, uint64_t def) const {
    uint64_t res{0};
    if (auto err = nvs_get_u64(_handle, key.data(), &res); err != ESP_OK) {
        return def;
    }
    return res;
}

void NvsStorage::commit() const {
    nvs_commit(_handle);
}

NvsStorage::~NvsStorage() {
    nvs_close(_handle);
}
