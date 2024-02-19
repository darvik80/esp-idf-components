FILE(GLOB_RECURSE srcs
        ${CMAKE_SOURCE_DIR}/main/*.cpp
)

idf_component_register(SRCS ${srcs} PRIV_REQUIRES unity cmock test_utils core-esp32)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
