//
// Created by Ivan Kishchenko on 08/08/2023.
//

#include "Logger.h"
#include "Properties.h"

void PropertiesLoader::load(std::string_view filePath) {
    std::string cfg;
    char buf[64];
    if (auto *file = fopen(filePath.data(), "r"); file) {
        esp_logi(props, "read props file: %s", filePath.data());
        while (fgets(buf, 64, file) != nullptr) {
            cfg.append(buf);
        }
        fclose(file);

        cJSON *json = cJSON_Parse(cfg.c_str());
        cJSON *item = json->child;
        while (item) {
            esp_logi(props, "read props: %s", item->string);
            if (item->type == cJSON_Object || item->type == cJSON_Array) {
                if (
                    auto it = _readers.find(item->string);it != _readers.end()) {
                    auto props = it->second(item);
                    for (
                        auto consumer: _consumers) {
                        consumer->applyProperties(*props);
                    }
                }
            }
            item = item->next;
        }
        cJSON_Delete(json);
    } else {
        esp_logi(props, "can't read props: %s", filePath.data());
    }
}