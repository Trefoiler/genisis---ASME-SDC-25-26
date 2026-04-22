// motor_test.cpp
// Exercises the motor and servo abstractions with live controller input.
// Tests 2 motors or 3 servos at a time. Cycle through pages with D-Pad left/right.
//
// Motor pages (left Y = motor A, right Y = motor B):
//   Page 1: FL, FR
//   Page 2: BL, BR
//   Page 3: STRAFE, AUX1
//   Page 4: AUX2, AUX3
//
// Servo page (page 5):
//   Left  stick Y         -> SERVO_1
//   Right stick Y         -> SERVO_2
//   Right trigger - Left  -> SERVO_3
//
// Motor values are squared before sending for finer low-speed control.
// Servo values are linear (direct position control).
// Servo page always prints all 3 positions at 100ms intervals.

#include <cstdio>
#include <pico/time.h>

extern "C" {
#include "controller_state.h"
#include "motor.h"
#include "servo.h"
#include "motor_test.h"
}

#define LOG_INTERVAL_MS  100
#define MOTORS_PER_PAGE  2
#define MOTOR_PAGE_COUNT (MOTOR_COUNT / MOTORS_PER_PAGE)  // 4
#define SERVO_PAGE       MOTOR_PAGE_COUNT                  // index 4
#define STRESS_PAGE      (MOTOR_PAGE_COUNT + 1)            // index 5
#define TOTAL_PAGE_COUNT (MOTOR_PAGE_COUNT + 2)            // 6

static const char* motor_names[MOTOR_COUNT] = {
    "FL", "FR", "BL", "BR", "STRAFE", "AUX1", "AUX2", "AUX3"
};

static int      s_page        = 0;
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

static void print_page(int page) {
    if (page < MOTOR_PAGE_COUNT) {
        MotorId a = (MotorId)(page * MOTORS_PER_PAGE);
        MotorId b = (MotorId)(page * MOTORS_PER_PAGE + 1);
        std::printf("motor_test: page %d/%d  left Y -> %s  right Y -> %s\n",
                    page + 1, (int)TOTAL_PAGE_COUNT, motor_names[a], motor_names[b]);
    } else if (page == SERVO_PAGE) {
        std::printf("motor_test: page %d/%d  left Y -> S1  right Y -> S2  triggers -> S3\n",
                    page + 1, (int)TOTAL_PAGE_COUNT);
    } else {
        std::printf("motor_test: page %d/%d  [STRESS TEST]  left Y -> all motors  right Y -> all servos\n",
                    page + 1, (int)TOTAL_PAGE_COUNT);
    }
}

extern "C" void motor_test_init(void) {
    s_page        = 0;
    s_was_stopped = true;
    s_last_log_ms = 0;
    print_page(s_page);
}

extern "C" void motor_test_update(void) {
    if (!controller_connected()) {
        return;
    }

    // Page navigation (edge-triggered so one press = one step)
    if (controller_button_pressed(CONTROLLER_BUTTON_DPAD_RIGHT)) {
        motors_stop_all();
        servos_center_all();
        s_page = (s_page + 1) % TOTAL_PAGE_COUNT;
        s_was_stopped = true;
        print_page(s_page);
        return;
    }
    if (controller_button_pressed(CONTROLLER_BUTTON_DPAD_LEFT)) {
        motors_stop_all();
        servos_center_all();
        s_page = (s_page - 1 + TOTAL_PAGE_COUNT) % TOTAL_PAGE_COUNT;
        s_was_stopped = true;
        print_page(s_page);
        return;
    }

    if (s_page == SERVO_PAGE) {
        float s1 = controller_left_y();
        float s2 = controller_right_y();
        float s3 = controller_right_trigger() - controller_left_trigger();

        servo_set_position(SERVO_1, s1);
        servo_set_position(SERVO_2, s2);
        servo_set_position(SERVO_3, s3);

        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if ((now_ms - s_last_log_ms) >= LOG_INTERVAL_MS) {
            s_last_log_ms = now_ms;
            std::printf("S1=%+d%%  S2=%+d%%  S3=%+d%%\n",
                        (int)(s1 * 100.0f), (int)(s2 * 100.0f), (int)(s3 * 100.0f));
        }
        return;
    }

    if (s_page == STRESS_PAGE) {
        float raw_m = controller_left_y();
        float raw_s = controller_right_y();
        float m     = square(raw_m);

        for (int i = 0; i < (int)MOTOR_COUNT; i++) motor_set_power((MotorId)i, m);
        for (int i = 0; i < (int)SERVO_COUNT; i++) servo_set_position((ServoId)i, raw_s);

        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if ((now_ms - s_last_log_ms) >= LOG_INTERVAL_MS) {
            s_last_log_ms = now_ms;
            std::printf("MOTORS=%+d%%(raw=%+d%%)  SERVOS=%+d%%\n",
                        (int)(m * 100.0f), (int)(raw_m * 100.0f), (int)(raw_s * 100.0f));
        }
        return;
    }

    // Motor pages
    MotorId id_a = (MotorId)(s_page * MOTORS_PER_PAGE);
    MotorId id_b = (MotorId)(s_page * MOTORS_PER_PAGE + 1);

    float raw_a = controller_left_y();
    float raw_b = controller_right_y();
    float a     = square(raw_a);
    float b     = square(raw_b);

    bool stopped = (a == 0.0f) && (b == 0.0f);

    if (stopped) {
        motors_stop_all();
        if (!s_was_stopped) {
            std::printf("motor_test: stopped\n");
            s_was_stopped = true;
        }
        return;
    }

    motor_set_power(id_a, a);
    motor_set_power(id_b, b);
    s_was_stopped = false;

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - s_last_log_ms) < LOG_INTERVAL_MS) {
        return;
    }
    s_last_log_ms = now_ms;

    bool printed = false;
    print_motor(motor_names[id_a], raw_a, a, &printed);
    print_motor(motor_names[id_b], raw_b, b, &printed);
    if (printed) std::printf("\n");
}
