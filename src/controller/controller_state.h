#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool connected;

    // Raw values from Bluepad32
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

// Virtual buttons used by the rest of the project.
// D-pad directions are included here too.
typedef enum {
    CONTROLLER_BUTTON_X = 0,
    CONTROLLER_BUTTON_CIRCLE,
    CONTROLLER_BUTTON_SQUARE,
    CONTROLLER_BUTTON_TRIANGLE,
    CONTROLLER_BUTTON_L1,
    CONTROLLER_BUTTON_R1,
    CONTROLLER_BUTTON_L2,
    CONTROLLER_BUTTON_R2,
    CONTROLLER_BUTTON_L3,
    CONTROLLER_BUTTON_R3,
    CONTROLLER_BUTTON_PS,
    CONTROLLER_BUTTON_SHARE,
    CONTROLLER_BUTTON_OPTIONS,
    CONTROLLER_BUTTON_DPAD_UP,
    CONTROLLER_BUTTON_DPAD_DOWN,
    CONTROLLER_BUTTON_DPAD_RIGHT,
    CONTROLLER_BUTTON_DPAD_LEFT,
    CONTROLLER_BUTTON_COUNT
} ControllerButton;

void controller_state_init(void);
void controller_state_set_disconnected(void);

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

const ControllerState* controller_state_get(void);

bool controller_connected(void);
unsigned int controller_battery_percent(void);

// Clean analog helpers
float controller_left_x(void);
float controller_left_y(void);
float controller_right_x(void);
float controller_right_y(void);
float controller_left_trigger(void);
float controller_right_trigger(void);

// Unified button helpers
bool controller_button_down(ControllerButton button);
bool controller_button_pressed(ControllerButton button);
bool controller_button_released(ControllerButton button);
const char* controller_button_name(ControllerButton button);

#ifdef __cplusplus
}
#endif

#endif