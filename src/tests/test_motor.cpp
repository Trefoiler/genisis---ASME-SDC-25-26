// test_motor.cpp
// Standalone motor test for DRV8871 via PWM.
// Ramps the motor forward, brakes, ramps reverse, brakes, and repeats.
//
// Wiring (DRV8871):
//   GP14 → IN1
//   GP15 → IN2
//   Pico GND → DRV8871 GND
//   Battery (+) → VMotor, Battery (-) → DRV8871 GND

#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>

// GPIO pins connected to DRV8871 IN1 and IN2.
// GP14 = PWM slice 7 channel A, GP15 = PWM slice 7 channel B.
#define MOTOR_IN1_PIN 14
#define MOTOR_IN2_PIN 15

// PWM configuration.
// RP2350 default system clock: 150 MHz
// clkdiv=15, wrap=9999 → 150MHz / 15 / 10000 = 1000 Hz
#define PWM_WRAP     9999u
#define PWM_CLKDIV   15.0f

static uint s_slice;
static uint s_chan_in1;
static uint s_chan_in2;

static void motor_init(void) {
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
}

// speed: -1.0 (full reverse) to +1.0 (full forward), 0.0 = coast
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

// Both IN pins HIGH = active brake
static void motor_brake(void) {
    pwm_set_chan_level(s_slice, s_chan_in1, PWM_WRAP);
    pwm_set_chan_level(s_slice, s_chan_in2, PWM_WRAP);
}

int main(void) {
    stdio_init_all();
    motor_init();

    std::printf("motor_test: starting\n");

    while (true) {
        // Ramp forward 0% → 100% over 5 seconds
        std::printf("motor_test: ramping up\n");
        for (int i = 0; i <= 100; i++) {
            motor_set(i / 100.0f);
            sleep_ms(50);
        }

        // Full power for 5 seconds
        std::printf("motor_test: full power\n");
        sleep_ms(5000);

        // Ramp down 100% → 0% over 5 seconds
        std::printf("motor_test: ramping down\n");
        for (int i = 100; i >= 0; i--) {
            motor_set(i / 100.0f);
            sleep_ms(50);
        }

        // Active brake for 3 seconds
        std::printf("motor_test: brake\n");
        motor_brake();
        sleep_ms(3000);

        // Free spin (coast) for 3 seconds
        std::printf("motor_test: coast\n");
        motor_set(0.0f);
        sleep_ms(3000);
    }
}
