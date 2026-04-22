// servo.cpp
// PPM servo abstraction for standard RC position servos (50Hz, 1-2ms).
// No arming sequence needed — servos respond immediately to signal.
//
// Pin assignments:
//   GP15 -> SERVO_1  (slice 7B)
//   GP14 -> SERVO_2  (slice 7A)
//   GP13 -> SERVO_3  (slice 6B)

#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

extern "C" {
#include "servo.h"
}

// Same PPM timing as motors: 150MHz / clkdiv(150) / (wrap+1)(20000) = 50Hz, 1 count = 1us
#define PWM_WRAP    19999u
#define PWM_CLKDIV  150.0f
#define PPM_NEUTRAL 1500u
#define PPM_RANGE    500u

#define RP2350_PWM_SLICES 12

struct ServoConfig {
    uint8_t pin;
};

static const ServoConfig servo_configs[SERVO_COUNT] = {
    [SERVO_1] = { 15 },
    [SERVO_2] = { 14 },
    [SERVO_3] = { 13 },
};

static struct {
    uint slice;
    uint channel;
} servo_state[SERVO_COUNT];

static uint position_to_counts(float position) {
    if (position >  1.0f) position =  1.0f;
    if (position < -1.0f) position = -1.0f;
    return (uint)((int)PPM_NEUTRAL + (int)(position * (float)PPM_RANGE));
}

extern "C" void servos_init(void) {
    bool slice_initialized[RP2350_PWM_SLICES] = {};

    for (int i = 0; i < SERVO_COUNT; i++) {
        uint pin = servo_configs[i].pin;
        gpio_set_function(pin, GPIO_FUNC_PWM);

        uint slice   = pwm_gpio_to_slice_num(pin);
        uint channel = pwm_gpio_to_channel(pin);
        servo_state[i].slice   = slice;
        servo_state[i].channel = channel;

        if (!slice_initialized[slice]) {
            pwm_config cfg = pwm_get_default_config();
            pwm_config_set_clkdiv(&cfg, PWM_CLKDIV);
            pwm_config_set_wrap(&cfg, PWM_WRAP);
            pwm_init(slice, &cfg, false);
            slice_initialized[slice] = true;
        }

        pwm_set_chan_level(slice, channel, PPM_NEUTRAL);
    }

    for (int s = 0; s < RP2350_PWM_SLICES; s++) {
        if (slice_initialized[s]) {
            pwm_set_enabled(s, true);
        }
    }

    std::printf("servos: init (%d servos)\n", (int)SERVO_COUNT);
}

extern "C" void servo_set_position(ServoId id, float position) {
    if (id < 0 || id >= SERVO_COUNT) return;
    pwm_set_chan_level(servo_state[id].slice, servo_state[id].channel, position_to_counts(position));
}

extern "C" void servos_center_all(void) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        pwm_set_chan_level(servo_state[i].slice, servo_state[i].channel, PPM_NEUTRAL);
    }
}
