cmake_minimum_required(VERSION 3.10)

file(GLOB SOURCES
        src/exchange/*.cpp
        src/exchange/spi/*.cpp
        src/exchange/i2c/*.cpp
        src/exchange/uart/*.cpp
        src/exchange/espnow/*.cpp
)

idf_component_register(
        SRCS
        ${SOURCES}
        INCLUDE_DIRS
        src
        REQUIRES
        core-esp32
)
