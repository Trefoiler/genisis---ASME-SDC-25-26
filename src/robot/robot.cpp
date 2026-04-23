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
//   Triggers       → arm open/close (SERVO_1 + SERVO_2 mirrored)
//   Triangle       → SERVO_3 to lifted position, switch to AIR
//   X              → SERVO_3 to ground position, switch to GROUND

#include <cstdio>

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

// -------------------------

static RobotState s_state = ROBOT_STATE_GROUND;

static float absf(float v) { return v < 0.0f ? -v : v; }

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
        float forward = controller_left_y();
        float strafe  = controller_left_x();
        float turn    = controller_right_x();

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
    } else {
        // AIR: wheels off ground, only H-drive responds to left stick X.
        motor_set_power(MOTOR_FL,     0.0f);
        motor_set_power(MOTOR_FR,     0.0f);
        motor_set_power(MOTOR_BL,     0.0f);
        motor_set_power(MOTOR_BR,     0.0f);
        motor_set_power(MOTOR_STRAFE, controller_left_x());
    }
}

static void update_arm_lift(void) {
    float power = 0.0f;
    if (controller_button_down(CONTROLLER_BUTTON_DPAD_UP))   power =  1.0f;
    if (controller_button_down(CONTROLLER_BUTTON_DPAD_DOWN)) power = -1.0f;
    motor_set_power(MOTOR_AUX1, power);
}

static void update_escalators(void) {
    motor_set_power(MOTOR_AUX2, controller_button_down(CONTROLLER_BUTTON_L1) ? 1.0f : 0.0f);
    motor_set_power(MOTOR_AUX3, controller_button_down(CONTROLLER_BUTTON_R1) ? 1.0f : 0.0f);
}

static void update_arm_servos(void) {
    // Positive arm_input = close, negative = open.
    // SERVO_2 is physically mirrored so it gets the opposite sign.
    float arm = controller_right_trigger() - controller_left_trigger();
    servo_set_position(SERVO_1,  arm);
    servo_set_position(SERVO_2, -arm);
}

extern "C" void robot_init(void) {
    s_state = ROBOT_STATE_GROUND;
    servo_set_position(SERVO_3, SERVO3_GROUND_POS);
    std::printf("robot: init, state=GROUND\n");
}

extern "C" void robot_update(void) {
    if (!controller_connected()) return;

    update_state_transitions();
    update_drive();
    update_arm_lift();
    update_escalators();
    update_arm_servos();
}

extern "C" RobotState robot_get_state(void) {
    return s_state;
}
