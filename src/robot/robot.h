#ifndef ROBOT_H
#define ROBOT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ROBOT_STATE_GROUND = 0,
    ROBOT_STATE_AIR,
} RobotState;

void robot_init(void);
void robot_update(void);
RobotState robot_get_state(void);

#ifdef __cplusplus
}
#endif

#endif
