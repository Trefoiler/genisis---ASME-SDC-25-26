// robot.cpp
// Main robot control logic. Called once per controller data frame.
//
// Two states (toggled by Triangle/X):
//   GROUND — normal driving with joysticks
//   AIR    — front of robot lifted; left stick X drives H-drive only
//
// State-independent controls (always active):
//   D-Pad up/down  → arm lift motor (AUX1)
//   L1 / R1        → escalator motors (AUX2 / AUX3)
//   Square         → unjam servo oscillates 0°↔180° while held, holds on release
//   Triggers       → arm open/close (SERVO_1 + SERVO_2 mirrored)
//   Triangle       → SERVO_3 to lifted position, switch to AIR
//   X              → SERVO_3 to ground position, switch to GROUND

#include <cstdio>
#include <pico/time.h>

extern "C" {
#include "robot.h"
#include "motor.h"
#include "servo.h"
#include "controller_state.h"
}

// --- Tunable constants ---

// Fraction of turn input added to H-drive when turning in GROUND state.
// Positive = H-drive pushes in the same lateral direction as the turn.
// Tune during testing; flip sign if robot rotates the wrong way.
#define H_DRIVE_TURN_COEFF   0.3f

// SERVO_3 positions for the front lift mechanism. Tune during testing.
#define SERVO3_LIFTED_POS    1.0f
#define SERVO3_GROUND_POS   -1.0f

// Minimum motor output when any joystick input is non-zero.
// Motors don't move below this threshold, so we scale the joystick range
// to start here instead of at 0. Tune if motors stall or jump too abruptly.
#define MOTOR_MIN_POWER      0.10f

#define DRIVE_LOG_INTERVAL_MS  100

#define CONVEYOR_MAX_POWER     0.60f

// Power applied to the arm lift motor when D-Pad up/down is held.
#define ARM_LIFT_POWER_UP    0.50f
#define ARM_LIFT_POWER_DOWN  0.30f

// Grabber travel speed in servo position units per second.
// At 1.0 the grabber takes 1 second to travel from fully open to fully closed.
// Tune if the claw moves too fast or too slow.
#define GRABBER_SPEED        2.667f


// Unjam servo oscillates between 0° and 180° on a 270° servo.
// Positions expressed in servo units (-1.0 = 0°, +1.0 = 270°).
// Tune UNJAM_MIN_POS/UNJAM_MAX_POS if the physical angles need adjusting.
#define UNJAM_MIN_POS        -1.0f
#define UNJAM_MAX_POS         1.0f
// Full cycle in 1 second: each half = 0.5s over 2.0 units
#define UNJAM_SPEED           4.0f

// -------------------------

static RobotState  s_state              = ROBOT_STATE_GROUND;
static bool        s_escalator_reversed = false;
static uint32_t    s_last_log_ms     = 0;
static float       s_grabber_pos     = 0.0f;
static uint64_t    s_grabber_last_us = 0;
static uint32_t    s_grabber_log_ms  = 0;
static float       s_unjam_pos       = UNJAM_MIN_POS;
static float       s_unjam_dir       = 1.0f;
static uint64_t    s_unjam_last_us   = 0;

static float absf(float v) { return v < 0.0f ? -v : v; }

// Squares the raw joystick input (for finer low-speed control) then scales
// the non-zero result so the output range is [MOTOR_MIN_POWER, 1.0] rather
// than [0, 1]. Zero input always produces zero output.
static float drive_curve(float raw) {
    float v = raw * raw * raw;       // cube, preserve sign
    if (v == 0.0f) return 0.0f;
    float sign = v < 0.0f ? -1.0f : 1.0f;
    float mag  = absf(v);
    return sign * (MOTOR_MIN_POWER + mag * (1.0f - MOTOR_MIN_POWER));
}

// Scale a set of drive values so the largest magnitude is at most 1.0,
// preserving the ratio between all values.
static void normalize5(float* fl, float* fr, float* bl, float* br, float* h) {
    float mx = 1.0f;
    if (absf(*fl) > mx) mx = absf(*fl);
    if (absf(*fr) > mx) mx = absf(*fr);
    if (absf(*bl) > mx) mx = absf(*bl);
    if (absf(*br) > mx) mx = absf(*br);
    if (absf(*h)  > mx) mx = absf(*h);
    *fl /= mx;
    *fr /= mx;
    *bl /= mx;
    *br /= mx;
    *h  /= mx;
}

static void update_state_transitions(void) {
    if (controller_button_pressed(CONTROLLER_BUTTON_TRIANGLE)) {
        s_state = ROBOT_STATE_AIR;
        servo_set_position(SERVO_3, SERVO3_LIFTED_POS);
        std::printf("robot: -> AIR\n");
    } else if (controller_button_pressed(CONTROLLER_BUTTON_X)) {
        s_state = ROBOT_STATE_GROUND;
        servo_set_position(SERVO_3, SERVO3_GROUND_POS);
        std::printf("robot: -> GROUND\n");
    }
}

static void update_drive(void) {
    if (s_state == ROBOT_STATE_GROUND) {
        float forward = drive_curve(controller_left_y());
        float strafe  = drive_curve(controller_left_x());
        float turn    = drive_curve(controller_right_x());

        // Differential drive: turning right adds to left side, subtracts from right.
        // H-drive contributes to turn because it is offset forward from the pivot.
        float fl = forward + turn;
        float fr = forward - turn;
        float bl = forward + turn;
        float br = forward - turn;
        float h  = strafe + turn * H_DRIVE_TURN_COEFF;

        normalize5(&fl, &fr, &bl, &br, &h);

        motor_set_power(MOTOR_FL,     fl);
        motor_set_power(MOTOR_FR,     fr);
        motor_set_power(MOTOR_BL,     bl);
        motor_set_power(MOTOR_BR,     br);
        motor_set_power(MOTOR_STRAFE, h);

        if (fl != 0.0f || fr != 0.0f || bl != 0.0f || br != 0.0f || h != 0.0f) {
            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            if ((now_ms - s_last_log_ms) >= DRIVE_LOG_INTERVAL_MS) {
                s_last_log_ms = now_ms;
                std::printf("drive: FL=%+d%%  FR=%+d%%  BL=%+d%%  BR=%+d%%  H=%+d%%\n",
                            (int)(fl * 100.0f), (int)(fr * 100.0f),
                            (int)(bl * 100.0f), (int)(br * 100.0f),
                            (int)(h  * 100.0f));
            }
        }
    } else {
        // AIR: wheels off ground, only H-drive responds to left stick X.
        motor_set_power(MOTOR_FL,     0.0f);
        motor_set_power(MOTOR_FR,     0.0f);
        motor_set_power(MOTOR_BL,     0.0f);
        motor_set_power(MOTOR_BR,     0.0f);
        float h = drive_curve(controller_left_x());
        motor_set_power(MOTOR_STRAFE, h);

        if (h != 0.0f) {
            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            if ((now_ms - s_last_log_ms) >= DRIVE_LOG_INTERVAL_MS) {
                s_last_log_ms = now_ms;
                std::printf("drive(air): H=%+d%%\n", (int)(h * 100.0f));
            }
        }
    }
}

static void update_arm_lift(void) {
    float power = 0.0f;
    if (controller_button_down(CONTROLLER_BUTTON_DPAD_UP))   power =  ARM_LIFT_POWER_UP;
    if (controller_button_down(CONTROLLER_BUTTON_DPAD_DOWN)) power = -ARM_LIFT_POWER_DOWN;
    motor_set_power(MOTOR_AUX1, power);
}

static void update_escalators(void) {
    if (controller_button_pressed(CONTROLLER_BUTTON_CIRCLE)) {
        s_escalator_reversed = !s_escalator_reversed;
        std::printf("escalators: %s\n", s_escalator_reversed ? "REVERSE" : "FORWARD");
    }

    float dir = s_escalator_reversed ? -1.0f : 1.0f;
    float lt  = controller_left_trigger();
    float rt  = controller_right_trigger();
    motor_set_power(MOTOR_AUX2, dir * lt * lt * lt * CONVEYOR_MAX_POWER);
    motor_set_power(MOTOR_AUX3, dir * rt * rt * rt * CONVEYOR_MAX_POWER);
}

static void update_unjam(void) {
    uint64_t now = time_us_64();
    float dt = (float)(now - s_unjam_last_us) * 1e-6f;
    if (dt > 0.1f) dt = 0.1f;
    s_unjam_last_us = now;

    if (!controller_button_down(CONTROLLER_BUTTON_SQUARE)) return;

    s_unjam_pos += UNJAM_SPEED * s_unjam_dir * dt;
    if (s_unjam_pos >= UNJAM_MAX_POS) {
        s_unjam_pos = UNJAM_MAX_POS;
        s_unjam_dir = -1.0f;
    } else if (s_unjam_pos <= UNJAM_MIN_POS) {
        s_unjam_pos = UNJAM_MIN_POS;
        s_unjam_dir = 1.0f;
    }
    servo_set_position(SERVO_UNJAM, s_unjam_pos);
}

static void update_arm_servos(void) {
    uint64_t now = time_us_64();
    float dt = (float)(now - s_grabber_last_us) * 1e-6f;
    if (dt > 0.1f) dt = 0.1f;
    s_grabber_last_us = now;

    // R1 closes (positive), L1 opens (negative).
    float input = (controller_button_down(CONTROLLER_BUTTON_R1) ? 1.0f : 0.0f)
                - (controller_button_down(CONTROLLER_BUTTON_L1) ? 1.0f : 0.0f);
    s_grabber_pos += input * GRABBER_SPEED * dt;
    if (s_grabber_pos >  1.0f) s_grabber_pos =  1.0f;
    if (s_grabber_pos < -1.0f) s_grabber_pos = -1.0f;

    // SERVO_2 is physically mirrored so it gets the opposite sign.
    servo_set_position(SERVO_1,  s_grabber_pos);
    servo_set_position(SERVO_2, -s_grabber_pos);

    if (input != 0.0f) {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if ((now_ms - s_grabber_log_ms) >= DRIVE_LOG_INTERVAL_MS) {
            s_grabber_log_ms = now_ms;
            std::printf("grabber: %+d%%\n", (int)(s_grabber_pos * 100.0f));
        }
    }
}

extern "C" void robot_init(void) {
    s_state           = ROBOT_STATE_GROUND;
    s_grabber_pos     = 0.0f;
    s_grabber_last_us = time_us_64();
    s_unjam_pos       = UNJAM_MIN_POS;
    s_unjam_dir       = 1.0f;
    s_unjam_last_us   = time_us_64();
    servo_set_position(SERVO_3, SERVO3_GROUND_POS);
    std::printf("robot: init, state=GROUND\n");
}

extern "C" void robot_update(void) {
    if (!controller_connected()) return;

    update_state_transitions();
    update_drive();
    update_arm_lift();
    update_escalators();
    update_unjam();
    update_arm_servos();
}

extern "C" RobotState robot_get_state(void) {
    return s_state;
}
