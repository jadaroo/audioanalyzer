cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(audioanalyzer C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# SSD1306 OLED driver (lives outside this tree, so it needs its own binary dir)
set(SSD1306_PATH ${CMAKE_CURRENT_LIST_DIR}/../fund/SSD1306 CACHE PATH "Path to the SSD1306 library")
set(SSD1306_BUILD_EXAMPLES OFF)
add_subdirectory(${SSD1306_PATH} ${CMAKE_BINARY_DIR}/ssd1306)

add_executable(${PROJECT_NAME}
    main.cpp
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
    ssd1306::ssd1306
    hardware_i2c
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
