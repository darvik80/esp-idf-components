cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")

file(GLOB SOURCES
        src/*.cpp
        src/sntp/*.cpp
)

idf_component_register(
        SRCS
            ${SOURCES}
        INCLUDE_DIRS
            src
        REQUIRES
            core-esp32
)
