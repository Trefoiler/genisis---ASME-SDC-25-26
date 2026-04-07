// test_controller_motor.cpp
// Uses the left joystick Y axis to control a motor via DRV8871.
// Nonzero power: drives motor and prints power at 100ms intervals.
// Zero power (joystick in deadzone): active brake, prints "brake" once on transition.
//
// Wiring (DRV8871):
//   GP14 → IN1
//   GP15 → IN2
//   Pico GND → DRV8871 GND
//   Battery (+) → VMotor, Battery (-) → DRV8871 GND

#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

extern "C" {
#include "controller_state.h"
#include "test_controller_motor.h"
}

#define MOTOR_IN1_PIN 14
#define MOTOR_IN2_PIN 15

// PWM: 150MHz / 15 / 10000 = 1kHz
#define PWM_WRAP    9999u
#define PWM_CLKDIV  15.0f

#define POWER_LOG_INTERVAL_MS 100

static uint s_slice;
static uint s_chan_in1;
static uint s_chan_in2;
static bool s_was_braking = true;
static uint32_t s_last_log_ms = 0;

static void motor_set(float speed) {
    if (speed > 1.0f)  speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;

    if (speed > 0.0f) {
        pwm_set_chan_level(s_slice, s_chan_in1, (uint)(speed * PWM_WRAP));
        pwm_set_chan_level(s_slice, s_chan_in2, 0);
    } else if (speed < 0.0f) {
        pwm_set_chan_level(s_slice, s_chan_in1, 0);
        pwm_set_chan_level(s_slice, s_chan_in2, (uint)(-speed * PWM_WRAP));
    } else {
        pwm_set_chan_level(s_slice, s_chan_in1, 0);
        pwm_set_chan_level(s_slice, s_chan_in2, 0);
    }
}

static void motor_brake(void) {
    pwm_set_chan_level(s_slice, s_chan_in1, PWM_WRAP);
    pwm_set_chan_level(s_slice, s_chan_in2, PWM_WRAP);
}

extern "C" void test_controller_motor_init(void) {
    gpio_set_function(MOTOR_IN1_PIN, GPIO_FUNC_PWM);
    gpio_set_function(MOTOR_IN2_PIN, GPIO_FUNC_PWM);

    s_slice    = pwm_gpio_to_slice_num(MOTOR_IN1_PIN);
    s_chan_in1 = pwm_gpio_to_channel(MOTOR_IN1_PIN);
    s_chan_in2 = pwm_gpio_to_channel(MOTOR_IN2_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_CLKDIV);
    pwm_config_set_wrap(&cfg, PWM_WRAP);
    pwm_init(s_slice, &cfg, false);

    pwm_set_chan_level(s_slice, s_chan_in1, 0);
    pwm_set_chan_level(s_slice, s_chan_in2, 0);
    pwm_set_enabled(s_slice, true);

    s_was_braking = true;
    s_last_log_ms = 0;
}

extern "C" void test_controller_motor_update(void) {
    if (!controller_connected()) {
        motor_brake();
        return;
    }

    float raw = controller_left_y();
    float power = raw * (raw < 0.0f ? -raw : raw);  // square magnitude, preserve sign

    if (power == 0.0f) {
        motor_brake();
        if (!s_was_braking) {
            std::printf("motor: brake\n");
            s_was_braking = true;
        }
        return;
    }

    motor_set(power);
    s_was_braking = false;

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - s_last_log_ms) >= POWER_LOG_INTERVAL_MS) {
        s_last_log_ms = now_ms;
        std::printf("motor: power=%d%%\n", (int)(power * 100.0f));
    }
}
