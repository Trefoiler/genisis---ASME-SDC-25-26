// test_esc_controller.cpp
// Controls two motors via Flipsky Brushed ESC using PPM signals.
// Left stick Y  → Motor 1 (GP14 / PPM1)
// Right stick Y → Motor 2 (GP15 / PPM2)
// Values are squared for fine low-speed control.
// Prints joystick and motor power values to serial when nonzero.
//
// Wiring:
//   GP14 → ESC signal pin 1
//   GP15 → ESC signal pin 2
//   Pico GND → ESC signal GND (− on 4-pin block)
//   LiPo → ESC B+/B−

#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

extern "C" {
#include "controller_state.h"
#include "test_esc_controller.h"
}

#define ESC_PPM1_PIN  14
#define ESC_PPM2_PIN  15

// PPM timing: 1 count = 1μs
// 150MHz / clkdiv(150) / wrap+1(20000) = 50Hz
#define PWM_WRAP     19999u
#define PWM_CLKDIV   150.0f
#define PPM_NEUTRAL  1500u   // 1.5ms = stop
#define PPM_MIN      1000u   // 1.0ms = full reverse
#define PPM_MAX      2000u   // 2.0ms = full forward
#define PPM_RANGE     500u   // counts from neutral to min/max

#define LOG_INTERVAL_MS  100
#define ARM_DELAY_MS    2000

static uint s_slice;
static uint s_chan_ppm1;
static uint s_chan_ppm2;
static bool s_armed = false;
static bool s_was_stopped = true;
static uint32_t s_last_log_ms = 0;

static uint power_to_counts(float power) {
    if (power > 1.0f)  power = 1.0f;
    if (power < -1.0f) power = -1.0f;
    return (uint)((int)PPM_NEUTRAL + (int)(power * (float)PPM_RANGE));
}

static void esc_set(float power1, float power2) {
    pwm_set_chan_level(s_slice, s_chan_ppm1, power_to_counts(power1));
    pwm_set_chan_level(s_slice, s_chan_ppm2, power_to_counts(power2));
}

static void esc_neutral(void) {
    pwm_set_chan_level(s_slice, s_chan_ppm1, PPM_NEUTRAL);
    pwm_set_chan_level(s_slice, s_chan_ppm2, PPM_NEUTRAL);
}

extern "C" void test_esc_controller_init(void) {
    gpio_set_function(ESC_PPM1_PIN, GPIO_FUNC_PWM);
    gpio_set_function(ESC_PPM2_PIN, GPIO_FUNC_PWM);

    s_slice     = pwm_gpio_to_slice_num(ESC_PPM1_PIN);
    s_chan_ppm1 = pwm_gpio_to_channel(ESC_PPM1_PIN);
    s_chan_ppm2 = pwm_gpio_to_channel(ESC_PPM2_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_CLKDIV);
    pwm_config_set_wrap(&cfg, PWM_WRAP);
    pwm_init(s_slice, &cfg, false);

    esc_neutral();
    pwm_set_enabled(s_slice, true);

    // Hold neutral to satisfy the ESC arming sequence before the BT loop starts.
    std::printf("esc: arming...\n");
    sleep_ms(ARM_DELAY_MS);
    std::printf("esc: armed\n");

    s_armed = true;
    s_was_stopped = true;
    s_last_log_ms = 0;
}

extern "C" void test_esc_controller_update(void) {
    if (!s_armed) return;

    if (!controller_connected()) {
        esc_neutral();
        return;
    }

    float raw1 = controller_left_y();
    float raw2 = controller_right_y();

    // Square magnitude, preserve sign for fine low-speed control.
    float power1 = raw1 * (raw1 < 0.0f ? -raw1 : raw1);
    float power2 = raw2 * (raw2 < 0.0f ? -raw2 : raw2);

    bool stopped = (power1 == 0.0f) && (power2 == 0.0f);

    if (stopped) {
        esc_neutral();
        if (!s_was_stopped) {
            std::printf("esc: stopped\n");
            s_was_stopped = true;
        }
        return;
    }

    esc_set(power1, power2);
    s_was_stopped = false;

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - s_last_log_ms) >= LOG_INTERVAL_MS) {
        s_last_log_ms = now_ms;

        int raw1_pct   = (int)(raw1   * 100.0f);
        int raw2_pct   = (int)(raw2   * 100.0f);
        int power1_pct = (int)(power1 * 100.0f);
        int power2_pct = (int)(power2 * 100.0f);

        std::printf(
            "stick(L=%+d%% R=%+d%%)  power(M1=%+d%% M2=%+d%%)\n",
            raw1_pct, raw2_pct, power1_pct, power2_pct
        );
    }
}
