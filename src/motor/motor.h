#ifndef MOTOR_H
#define MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOTOR_FL = 0,  // front left
    MOTOR_FR,      // front right
    MOTOR_BL,      // back left
    MOTOR_BR,      // back right
    MOTOR_STRAFE,  // H-drive (strafe)
    MOTOR_COUNT
} MotorId;

// Initialize PWM for all motors and arm all ESCs.
// Must be called once before any motor_set_power() calls.
void motors_init(void);

// Set motor power. power is -1.0 (full reverse) to +1.0 (full forward), 0.0 = neutral.
void motor_set_power(MotorId id, float power);

// Send neutral PPM to all motors (ESC-safe stop).
void motors_stop_all(void);

#ifdef __cplusplus
}
#endif

#endif
