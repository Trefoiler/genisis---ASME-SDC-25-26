#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include <stdbool.h>
#include <stdint.h>

// Raw controller data straight from Bluepad32.
// Keep this around so we always have the original values if needed.
typedef struct {
    bool connected;

    uint8_t dpad;

    int32_t x;
    int32_t y;
    int32_t rx;
    int32_t ry;

    int32_t brake;
    int32_t throttle;

    uint16_t buttons;
    uint8_t misc;

    int32_t gyro_x;
    int32_t gyro_y;
    int32_t gyro_z;

    int32_t accel_x;
    int32_t accel_y;
    int32_t accel_z;

    uint8_t battery;
} ControllerState;

// Reset everything to a safe default state.
void controller_state_init(void);

// Mark the controller as disconnected and clear all inputs.
void controller_state_set_disconnected(void);

// Store the newest raw controller values.
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

// Read the latest raw state.
const ControllerState* controller_state_get(void);

// Clean helper functions for robot code.
// Sticks return -1.0 to 1.0.
// Triggers return 0.0 to 1.0.
// Y axes are flipped so up is positive.
float controller_left_x(void);
float controller_left_y(void);
float controller_right_x(void);
float controller_right_y(void);
float controller_left_trigger(void);
float controller_right_trigger(void);

#endif