cmake_minimum_required(VERSION 3.10)

file(GLOB SOURCES
        src/core/*.cpp
        src/core/system/mqtt/*.cpp
        src/core/system/wifi/*.cpp
        src/core/system/console/*.cpp
        src/core/system/telemetry/*.cpp
)

idf_component_register(
    SRCS
        ${SOURCES}
    INCLUDE_DIRS
        src
    REQUIRES
        esp_timer
        esp_event
        esp_wifi
        console
        nvs_flash
        mqtt
        spiffs
        json
        driver
)
