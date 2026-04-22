#ifndef SERVO_H
#define SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SERVO_1 = 0,
    SERVO_2,
    SERVO_3,
    SERVO_COUNT
} ServoId;

// Initialize PWM for all servos and center them.
// Must be called once before any servo_set_position() calls.
void servos_init(void);

// Set servo position. position is -1.0 (full one way) to +1.0 (full other way), 0.0 = center.
void servo_set_position(ServoId id, float position);

// Send neutral PPM to all servos (centers them).
void servos_center_all(void);

#ifdef __cplusplus
}
#endif

#endif
