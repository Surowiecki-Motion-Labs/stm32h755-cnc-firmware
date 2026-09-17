#ifndef AXIS_H
#define AXIS_H

#include "axis_t.h"


#ifdef __cplusplus
extern "C" {
#endif

extern axis_t axes[AXIS_COUNT];

void axis_init(void);
uint8_t axis_check(axis_id_t axis);

void axis_set_pos(axis_id_t axis, int32_t position_steps);
void axis_set_target(axis_id_t axis, int32_t target_steps);
void axis_set_is_homed(axis_id_t axis, uint8_t is_homed);

int32_t axis_get_pos(axis_id_t axis);
int32_t axis_get_target(axis_id_t axis);
uint8_t axis_get_is_homed(axis_id_t axis);

uint8_t axis_all_is_homed(void);
int32_t axis_get_config_home_offset(axis_id_t axis);


int32_t axis_um_to_steps(axis_id_t axis, int32_t um);
int32_t axis_steps_to_um(axis_id_t axis, int32_t steps);

#ifdef __cplusplus
}
#endif

#endif /* AXIS_H */
