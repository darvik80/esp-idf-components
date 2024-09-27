cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")

if (${IDF_TARGET} STREQUAL "linux")
    file(GLOB SOURCES
            src/core/v2/*.cpp
            src/core/EventBus.cpp
            src/core/Properties.cpp
            src/core/Timer.cpp
            src/core/Task.cpp
            src/core/system/telemetry/*.cpp
    )

    idf_component_register(
            SRCS
            ${SOURCES}
            INCLUDE_DIRS
            src
            REQUIRES
            esp_event
            nvs_flash
            spiffs
            json
    )
else ()
    file(GLOB SOURCES
            src/core/*.cpp
            src/core/v2/*.cpp
            src/core/system/mqtt/*.cpp
            src/core/system/wifi/*.cpp
            src/core/system/console/*.cpp
            src/core/system/telemetry/*.cpp
            src/core/system/ota/*.cpp
            src/core/system/storage/*.cpp
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
            esp_https_ota
            console
            nvs_flash
            mqtt
            spiffs
            json
            driver
            fmt
    )
endif ()