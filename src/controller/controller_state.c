#include "controller_state.h"

// Shared controller state used by the rest of the robot code.
static ControllerState g_controller_state;

// Observed DS4 stick range was about -508 to 512, with center near 4.
// We subtract the center offset, then scale to about -1 to 1.
#define STICK_CENTER_OFFSET 4
#define STICK_SCALE 512.0f

// Observed trigger max was about 1020.
#define TRIGGER_SCALE 1020.0f

// Ignore tiny stick noise around center.
#define STICK_DEADZONE 0.03f

static float clampf(float value, float min, float max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static float apply_deadzone(float value, float deadzone) {
    if (value > -deadzone && value < deadzone) {
        return 0.0f;
    }
    return value;
}

static float normalize_stick(int32_t raw_value) {
    // Shift the center so resting stick values land closer to zero.
    float shifted = (float)(raw_value - STICK_CENTER_OFFSET);

    // Convert to roughly -1.0 to 1.0.
    float normalized = shifted / STICK_SCALE;

    // Clamp in case the controller goes slightly past the expected range.
    normalized = clampf(normalized, -1.0f, 1.0f);

    // Kill tiny drift near center.
    normalized = apply_deadzone(normalized, STICK_DEADZONE);

    return normalized;
}

static float normalize_trigger(int32_t raw_value) {
    // Convert 0..1020 to 0.0..1.0.
    float normalized = (float)raw_value / TRIGGER_SCALE;
    return clampf(normalized, 0.0f, 1.0f);
}

void controller_state_init(void) {
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
    // For now, disconnected just means reset everything.
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
    g_controller_state.connected = true;

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
    return &g_controller_state;
}

float controller_left_x(void) {
    return normalize_stick(g_controller_state.x);
}

float controller_left_y(void) {
    // Raw DS4 Y is negative when pushed up, so flip it.
    return -normalize_stick(g_controller_state.y);
}

float controller_right_x(void) {
    return normalize_stick(g_controller_state.rx);
}

float controller_right_y(void) {
    // Raw DS4 Y is negative when pushed up, so flip it.
    return -normalize_stick(g_controller_state.ry);
}

float controller_left_trigger(void) {
    return normalize_trigger(g_controller_state.brake);
}

float controller_right_trigger(void) {
    return normalize_trigger(g_controller_state.throttle);
}