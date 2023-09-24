cmake_minimum_required(VERSION 3.10)

file(GLOB SOURCES src/bluetooth/*.cpp)

idf_component_register(
    SRCS
        ${SOURCES}
    INCLUDE_DIRS
        src
    REQUIRES
        core-esp32
        bt
        esp_hid
)
