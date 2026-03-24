#include "controller_state.h"

// This file owns the single shared copy of controller data.
// Bluepad32 writes into it, and the rest of the robot can read from it.

static ControllerState g_controller_state;

void controller_state_init(void) {
    // Start with everything zeroed and disconnected.
    g_controller_state.connected = false;
    g_controller_state.dpad = 0;
    g_controller_state.x = 0;
    g_controller_state.y = 0;
    g_controller_state.rx = 0;
    g_controller_state.ry = 0;
    g_controller_state.brake = 0;
    g_controller_state.throttle = 0;
    g_controller_state.buttons = 0;
    g_controller_state.misc = 0;
    g_controller_state.gyro_x = 0;
    g_controller_state.gyro_y = 0;
    g_controller_state.gyro_z = 0;
    g_controller_state.accel_x = 0;
    g_controller_state.accel_y = 0;
    g_controller_state.accel_z = 0;
    g_controller_state.battery = 0;
}

void controller_state_set_disconnected(void) {
    // Keep this separate so disconnect handling is simple and explicit.
    controller_state_init();
}

void controller_state_update(uint8_t dpad,
                             int32_t x,
                             int32_t y,
                             int32_t rx,
                             int32_t ry,
                             int32_t brake,
                             int32_t throttle,
                             uint16_t buttons,
                             uint8_t misc,
                             int32_t gyro_x,
                             int32_t gyro_y,
                             int32_t gyro_z,
                             int32_t accel_x,
                             int32_t accel_y,
                             int32_t accel_z,
                             uint8_t battery) {
    // Once we get valid data, the controller is connected.
    g_controller_state.connected = true;

    // Copy the latest controller values into our shared state.
    g_controller_state.dpad = dpad;
    g_controller_state.x = x;
    g_controller_state.y = y;
    g_controller_state.rx = rx;
    g_controller_state.ry = ry;
    g_controller_state.brake = brake;
    g_controller_state.throttle = throttle;
    g_controller_state.buttons = buttons;
    g_controller_state.misc = misc;

    g_controller_state.gyro_x = gyro_x;
    g_controller_state.gyro_y = gyro_y;
    g_controller_state.gyro_z = gyro_z;

    g_controller_state.accel_x = accel_x;
    g_controller_state.accel_y = accel_y;
    g_controller_state.accel_z = accel_z;

    g_controller_state.battery = battery;
}

const ControllerState* controller_state_get(void) {
    // Return read-only access to the latest controller values.
    return &g_controller_state;
}