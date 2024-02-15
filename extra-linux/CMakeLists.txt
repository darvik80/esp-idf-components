cmake_minimum_required(VERSION 3.10)

#if (NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/esp-mqtt/CMakeLists.txt)
#    set(COMMAND "git submodule update --init --recursive")
#    message(FATAL_ERROR "Missing hidapi library. Please run '${COMMAND}' in ESP-IDF directory to fix this.")
#else ()
#    include(${CMAKE_CURRENT_LIST_DIR}/esp-mqtt/CMakeLists.txt)
#endif ()


idf_build_get_property(target IDF_TARGET)
if (NOT ${target} STREQUAL "linux")
    return()
endif ()

file(GLOB SOURCES
        src/*.cpp
)

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS src
        REQUIRES core-esp32
)

include_directories(
        /usr/local/include
)

find_package(hidapi REQUIRED)

target_link_libraries(${COMPONENT_LIB} INTERFACE hidapi::hidapi)
return()

