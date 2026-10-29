cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(audioanalyzer C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    main.c
    src/initialization.c
    src/dft.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_adc
    hardware_dma
    hardware_irq
    m
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
