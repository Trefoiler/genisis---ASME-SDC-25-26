cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w)

set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../bluepad32)
set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

include(pico_sdk_import.cmake)

project(genisis C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    src/main.c
    src/controller/bluepad32_platform.c
    src/controller/controller_state.c
    src/tests/test_input.cpp
    src/tests/test_esc_controller.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/controller
    src/tests
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include
)

include_directories(
    src
    src/controller
    src/tests
)

include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
include_directories(${BTSTACK_ROOT}/src)

target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    hardware_pwm
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_cyw43
    bluepad32
)

add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})

# Standalone motor test (DRV8871, no Bluepad32)
add_executable(motor_test
    src/tests/test_motor.cpp
)

target_link_libraries(motor_test PUBLIC
    pico_stdlib
    hardware_pwm
)

pico_enable_stdio_usb(motor_test 1)
pico_enable_stdio_uart(motor_test 0)

pico_add_extra_outputs(motor_test)