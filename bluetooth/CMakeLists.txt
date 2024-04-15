cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")

file(GLOB SOURCES src/bluetooth/*.cpp src/beacon/*.cpp)

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
