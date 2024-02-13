cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)

project(smbus-slave C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(PROJECT_LIB ${PROJECT_NAME})
set(PROJECT_FIRMWARE ${PICO_BOARD}-${PROJECT_NAME})

include(lwip_import.cmake)
pico_sdk_init()


# Library
add_library(${PROJECT_LIB}
    lib/smbus_slave.c
)
target_link_libraries(${PROJECT_LIB} PRIVATE
    hardware_i2c
)
target_include_directories(${PROJECT_LIB} PRIVATE
    "${PROJECT_ROOT}/include"
)
target_compile_options(${PROJECT_LIB} PRIVATE -Wall)


# Firmware
add_executable(${PROJECT_FIRMWARE}
    firmware/main.c
    firmware/commands.h
)
target_link_libraries(${PROJECT_FIRMWARE} PRIVATE
    pico_stdlib
    hardware_i2c
    pico_cyw43_arch_none
    lwip_port
    ${PROJECT_LIB}
)
target_include_directories(${PROJECT_FIRMWARE} PRIVATE
    "${PROJECT_ROOT}/include"
)
target_compile_options(${PROJECT_FIRMWARE} PRIVATE -Wall)


# Run the entire project in SRAM
# pico_set_binary_type(pico-freertos copy_to_ram)

pico_add_extra_outputs(${PROJECT_FIRMWARE})

pico_enable_stdio_usb(${PROJECT_FIRMWARE} 0)
pico_enable_stdio_uart(${PROJECT_FIRMWARE} 1)