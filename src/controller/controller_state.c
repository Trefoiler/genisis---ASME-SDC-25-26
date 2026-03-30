#include "controller_state.h"

static ControllerState g_controller_state;

// Previous and current virtual button masks
static uint32_t g_prev_button_mask = 0;
static uint32_t g_curr_button_mask = 0;

// Observed DS4 values
#define STICK_CENTER_OFFSET 4
#define STICK_SCALE 512.0f
#define TRIGGER_SCALE 1020.0f
#define STICK_DEADZONE 0.01f // 1% joystick deadzone

// Raw DS4 button bits from your testing
#define RAW_BTN_X               0x0001
#define RAW_BTN_CIRCLE          0x0002
#define RAW_BTN_SQUARE          0x0004
#define RAW_BTN_TRIANGLE        0x0008
#define RAW_BTN_L1              0x0010
#define RAW_BTN_R1              0x0020
#define RAW_BTN_L2_BUTTON       0x0040
#define RAW_BTN_R2_BUTTON       0x0080
#define RAW_BTN_L3              0x0100
#define RAW_BTN_R3              0x0200

// Raw misc bits from your testing
#define RAW_MISC_PS             0x01
#define RAW_MISC_SHARE          0x02
#define RAW_MISC_OPTIONS        0x04

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
    float shifted = (float)(raw_value - STICK_CENTER_OFFSET);
    float normalized = shifted / STICK_SCALE;
    normalized = clampf(normalized, -1.0f, 1.0f);
    normalized = apply_deadzone(normalized, STICK_DEADZONE);
    return normalized;
}

static float normalize_trigger(int32_t raw_value) {
    float normalized = (float)raw_value / TRIGGER_SCALE;
    return clampf(normalized, 0.0f, 1.0f);
}

static uint32_t button_bit(ControllerButton button) {
    if (button < 0 || button >= CONTROLLER_BUTTON_COUNT) {
        return 0;
    }
    return 1u << (uint32_t)button;
}

static uint32_t build_virtual_button_mask(void) {
    uint32_t mask = 0;

    if (g_controller_state.buttons & RAW_BTN_X)         mask |= button_bit(CONTROLLER_BUTTON_X);
    if (g_controller_state.buttons & RAW_BTN_CIRCLE)    mask |= button_bit(CONTROLLER_BUTTON_CIRCLE);
    if (g_controller_state.buttons & RAW_BTN_SQUARE)    mask |= button_bit(CONTROLLER_BUTTON_SQUARE);
    if (g_controller_state.buttons & RAW_BTN_TRIANGLE)  mask |= button_bit(CONTROLLER_BUTTON_TRIANGLE);
    if (g_controller_state.buttons & RAW_BTN_L1)        mask |= button_bit(CONTROLLER_BUTTON_L1);
    if (g_controller_state.buttons & RAW_BTN_R1)        mask |= button_bit(CONTROLLER_BUTTON_R1);
    if (g_controller_state.buttons & RAW_BTN_L2_BUTTON) mask |= button_bit(CONTROLLER_BUTTON_L2);
    if (g_controller_state.buttons & RAW_BTN_R2_BUTTON) mask |= button_bit(CONTROLLER_BUTTON_R2);
    if (g_controller_state.buttons & RAW_BTN_L3)        mask |= button_bit(CONTROLLER_BUTTON_L3);
    if (g_controller_state.buttons & RAW_BTN_R3)        mask |= button_bit(CONTROLLER_BUTTON_R3);

    if (g_controller_state.misc & RAW_MISC_PS)          mask |= button_bit(CONTROLLER_BUTTON_PS);
    if (g_controller_state.misc & RAW_MISC_SHARE)       mask |= button_bit(CONTROLLER_BUTTON_SHARE);
    if (g_controller_state.misc & RAW_MISC_OPTIONS)     mask |= button_bit(CONTROLLER_BUTTON_OPTIONS);

    // D-pad directions as virtual buttons
    if (g_controller_state.dpad & 0x01)                 mask |= button_bit(CONTROLLER_BUTTON_DPAD_UP);
    if (g_controller_state.dpad & 0x02)                 mask |= button_bit(CONTROLLER_BUTTON_DPAD_DOWN);
    if (g_controller_state.dpad & 0x04)                 mask |= button_bit(CONTROLLER_BUTTON_DPAD_RIGHT);
    if (g_controller_state.dpad & 0x08)                 mask |= button_bit(CONTROLLER_BUTTON_DPAD_LEFT);

    return mask;
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

    g_prev_button_mask = 0;
    g_curr_button_mask = 0;
}

void controller_state_set_disconnected(void) {
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
    g_prev_button_mask = g_curr_button_mask;

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

    g_curr_button_mask = build_virtual_button_mask();
}

const ControllerState* controller_state_get(void) {
    return &g_controller_state;
}

bool controller_connected(void) {
    return g_controller_state.connected;
}

unsigned int controller_battery_percent(void) {
    return (g_controller_state.battery * 100u) / 255u;
}

float controller_left_x(void) {
    return normalize_stick(g_controller_state.x);
}

float controller_left_y(void) {
    return -normalize_stick(g_controller_state.y);
}

float controller_right_x(void) {
    return normalize_stick(g_controller_state.rx);
}

float controller_right_y(void) {
    return -normalize_stick(g_controller_state.ry);
}

float controller_left_trigger(void) {
    return normalize_trigger(g_controller_state.brake);
}

float controller_right_trigger(void) {
    return normalize_trigger(g_controller_state.throttle);
}

bool controller_button_down(ControllerButton button) {
    return (g_curr_button_mask & button_bit(button)) != 0;
}

bool controller_button_pressed(ControllerButton button) {
    uint32_t bit = button_bit(button);
    return ((g_curr_button_mask & bit) != 0) && ((g_prev_button_mask & bit) == 0);
}

bool controller_button_released(ControllerButton button) {
    uint32_t bit = button_bit(button);
    return ((g_curr_button_mask & bit) == 0) && ((g_prev_button_mask & bit) != 0);
}

const char* controller_button_name(ControllerButton button) {
    switch (button) {
        case CONTROLLER_BUTTON_X:          return "x";
        case CONTROLLER_BUTTON_CIRCLE:     return "circle";
        case CONTROLLER_BUTTON_SQUARE:     return "square";
        case CONTROLLER_BUTTON_TRIANGLE:   return "triangle";
        case CONTROLLER_BUTTON_L1:         return "l1";
        case CONTROLLER_BUTTON_R1:         return "r1";
        case CONTROLLER_BUTTON_L2:         return "l2";
        case CONTROLLER_BUTTON_R2:         return "r2";
        case CONTROLLER_BUTTON_L3:         return "l3";
        case CONTROLLER_BUTTON_R3:         return "r3";
        case CONTROLLER_BUTTON_PS:         return "ps";
        case CONTROLLER_BUTTON_SHARE:      return "share";
        case CONTROLLER_BUTTON_OPTIONS:    return "options";
        case CONTROLLER_BUTTON_DPAD_UP:    return "dpad_up";
        case CONTROLLER_BUTTON_DPAD_DOWN:  return "dpad_down";
        case CONTROLLER_BUTTON_DPAD_RIGHT: return "dpad_right";
        case CONTROLLER_BUTTON_DPAD_LEFT:  return "dpad_left";
        default:                           return "unknown";
    }
}