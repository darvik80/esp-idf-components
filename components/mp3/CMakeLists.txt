cmake_minimum_required(VERSION 3.10)

file(GLOB LIB_SRC src/*.c)

idf_component_register(
    SRCS
        ${LIB_SRC}
    INCLUDE_DIRS
        src
    REQUIRES
        driver
)
