#This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/tools/unit-test-app/components")
list(APPEND EXTRA_COMPONENT_DIRS "../../core-esp32")
list(APPEND EXTRA_COMPONENT_DIRS "../../fmt")

set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(core-esp32_test)
