// motor_test.cpp
// Exercises the motor abstraction with live controller input.
// Use this to verify each motor responds correctly before writing drive logic.
//
// Controls (each axis drives one motor independently for isolated testing):
//   Left  stick Y -> MOTOR_FL
//   Left  stick X -> MOTOR_BL
//   Right stick Y -> MOTOR_FR
//   Right stick X -> MOTOR_BR
//   Right trigger - Left trigger -> MOTOR_STRAFE  (positive = right trigger, negative = left)
//
// All values are squared before sending to the motor for finer low-speed control.
// Prints raw and squared values at 100ms intervals for any active motor.

#include <cstdio>
#include <pico/time.h>

extern "C" {
#include "controller_state.h"
#include "motor.h"
#include "motor_test.h"
}

#define LOG_INTERVAL_MS 100

static bool     s_was_stopped = true;
static uint32_t s_last_log_ms = 0;

static float square(float v) {
    return v * (v < 0.0f ? -v : v);
}

static void print_motor(const char* name, float raw, float power, bool* printed) {
    if (power == 0.0f) return;
    if (*printed) std::printf("  ");
    std::printf("%s=%+d%%(raw=%+d%%)", name, (int)(power * 100.0f), (int)(raw * 100.0f));
    *printed = true;
}

extern "C" void motor_test_init(void) {
    s_was_stopped = true;
    s_last_log_ms = 0;
}

extern "C" void motor_test_update(void) {
    if (!controller_connected()) {
        return;
    }

    float raw_fl = controller_left_y();
    float raw_bl = controller_left_x();
    float raw_fr = controller_right_y();
    float raw_br = controller_right_x();
    float raw_h  = controller_right_trigger() - controller_left_trigger();

    float fl = square(raw_fl);
    float bl = square(raw_bl);
    float fr = square(raw_fr);
    float br = square(raw_br);
    float h  = square(raw_h);

    bool stopped = (fl == 0.0f) && (bl == 0.0f) && (fr == 0.0f) && (br == 0.0f) && (h == 0.0f);

    if (stopped) {
        motors_stop_all();
        if (!s_was_stopped) {
            std::printf("motor_test: stopped\n");
            s_was_stopped = true;
        }
        return;
    }

    motor_set_power(MOTOR_FL, fl);
    motor_set_power(MOTOR_BL, bl);
    motor_set_power(MOTOR_FR, fr);
    motor_set_power(MOTOR_BR, br);
    motor_set_power(MOTOR_STRAFE, h);
    s_was_stopped = false;

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - s_last_log_ms) < LOG_INTERVAL_MS) {
        return;
    }
    s_last_log_ms = now_ms;

    // Print only active motors, with both raw and squared values.
    bool printed = false;
    print_motor("FL", raw_fl, fl, &printed);
    print_motor("BL", raw_bl, bl, &printed);
    print_motor("FR", raw_fr, fr, &printed);
    print_motor("BR", raw_br, br, &printed);
    print_motor("H",  raw_h,  h,  &printed);

    if (printed) std::printf("\n");
}
