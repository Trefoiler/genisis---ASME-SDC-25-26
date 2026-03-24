#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include <stdbool.h>
#include <stdint.h>

// This struct stores the latest controller data in a simple, project-owned format.
// The goal is that the rest of your robot code reads this struct instead of touching
// Bluepad32 types directly.
typedef struct {
    bool connected;

    // Basic gamepad data
    uint8_t dpad;
    int32_t x;
    int32_t y;
    int32_t rx;
    int32_t ry;
    int32_t brake;
    int32_t throttle;
    uint16_t buttons;
    uint8_t misc;

    // Motion / extra data
    int32_t gyro_x;
    int32_t gyro_y;
    int32_t gyro_z;
    int32_t accel_x;
    int32_t accel_y;
    int32_t accel_z;
    uint8_t battery;
} ControllerState;

// Reset the stored controller state to safe defaults.
void controller_state_init(void);

// Mark the controller as disconnected and clear all input values.
void controller_state_set_disconnected(void);

// Update the stored controller state with the newest values.
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
                             uint8_t battery);

// Get a pointer to the latest controller state.
const ControllerState* controller_state_get(void);

#endif