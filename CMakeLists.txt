cmake_minimum_required(VERSION 3.13)

# We are targeting the Pico 2 W.
set(PICO_BOARD pico2_w)

# Bluepad32 is next to this project folder.
set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../bluepad32)

# Use BTstack from the Pico SDK.
set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

# Must come before project()
include(pico_sdk_import.cmake)

project(genisis C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK.
pico_sdk_init()

# These are the source files that make up the firmware.
# This is what decides what actually gets built and run on the Pico.
add_executable(${PROJECT_NAME}
    src/main.c
    src/my_platform.c
    src/controller_state.c
)

# Project includes
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include
)

# Needed so Bluepad32 can see sdkconfig.h / btstack_config.h in src/
include_directories(src)

# Needed for BTstack headers
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
include_directories(${BTSTACK_ROOT}/src)

# Link Pico + Bluetooth + Bluepad32 libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_cyw43
    bluepad32
)

# Add the Bluepad32 library itself
add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

# Print over USB serial
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create .uf2 / .bin / .elf outputs
pico_add_extra_outputs(${PROJECT_NAME})