// motor.cpp
// PPM motor abstraction for Flipsky Brushed ESC.
// Each motor maps to a GPIO pin; pairs on the same PWM slice share timing.
//
// Pin assignments (consecutive even/odd pairs share a slice):
//   GP2 → MOTOR_FL      (ESC1 PPM1, slice 1A)
//   GP3 → MOTOR_FR      (ESC1 PPM2, slice 1B)
//   GP4 → MOTOR_BL      (ESC2 PPM1, slice 2A)
//   GP5 → MOTOR_BR      (ESC2 PPM2, slice 2B)
//   GP6 → MOTOR_STRAFE  (ESC3 PPM1, slice 3A)
//   GP7 → MOTOR_AUX1    (ESC3 PPM2, slice 3B)
//   GP8 → MOTOR_AUX2    (ESC4 PPM1, slice 4A)
//   GP9 → MOTOR_AUX3    (ESC4 PPM2, slice 4B)

#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

extern "C" {
#include "motor.h"
}

// PPM timing: 150MHz / clkdiv(150) / (wrap+1)(20000) = 50Hz, 1 count = 1μs
#define PWM_WRAP     19999u
#define PWM_CLKDIV   150.0f
#define PPM_NEUTRAL  1500u   // 1.5ms = stop
#define PPM_RANGE     500u   // counts from neutral to min/max
#define ARM_DELAY_MS 2000

#define RP2350_PWM_SLICES 12

struct MotorConfig {
    uint8_t pin;
    bool    invert;
};

static const MotorConfig motor_configs[MOTOR_COUNT] = {
    [MOTOR_FL] = { 2, false },
    [MOTOR_FR] = { 3, false },
    [MOTOR_BL] = { 4, false },
    [MOTOR_BR] = { 5, false },
    [MOTOR_STRAFE] = { 6, false },
    [MOTOR_AUX1]   = { 7, false },
    [MOTOR_AUX2]   = { 8, false },
    [MOTOR_AUX3]   = { 9, false },
};

static struct {
    uint slice;
    uint channel;
} motor_state[MOTOR_COUNT];

static uint power_to_counts(float power) {
    if (power >  1.0f) power =  1.0f;
    if (power < -1.0f) power = -1.0f;
    return (uint)((int)PPM_NEUTRAL + (int)(power * (float)PPM_RANGE));
}

extern "C" void motors_init(void) {
    bool slice_initialized[RP2350_PWM_SLICES] = {};

    for (int i = 0; i < MOTOR_COUNT; i++) {
        uint pin = motor_configs[i].pin;
        gpio_set_function(pin, GPIO_FUNC_PWM);

        uint slice   = pwm_gpio_to_slice_num(pin);
        uint channel = pwm_gpio_to_channel(pin);
        motor_state[i].slice   = slice;
        motor_state[i].channel = channel;

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

    std::printf("motors: arming...\n");
    sleep_ms(ARM_DELAY_MS);
    std::printf("motors: armed (%d motors)\n", (int)MOTOR_COUNT);
}

extern "C" void motor_set_power(MotorId id, float power) {
    if (id < 0 || id >= MOTOR_COUNT) return;
    if (motor_configs[id].invert) power = -power;
    pwm_set_chan_level(motor_state[id].slice, motor_state[id].channel, power_to_counts(power));
}

extern "C" void motors_stop_all(void) {
    for (int i = 0; i < MOTOR_COUNT; i++) {
        pwm_set_chan_level(motor_state[i].slice, motor_state[i].channel, PPM_NEUTRAL);
    }
}
