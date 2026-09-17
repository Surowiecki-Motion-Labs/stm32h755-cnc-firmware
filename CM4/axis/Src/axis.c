#include "axis.h"
#include "config_cm4.h"

#include <stdint.h>

axis_t axes[AXIS_COUNT];

static const int32_t axis_driver_pulses_per_rev[AXIS_COUNT] =
{
    [AXIS_X] = X_DRIVER_PULSES_PER_REV,
    [AXIS_Y] = Y_DRIVER_PULSES_PER_REV,
    [AXIS_Z] = Z_DRIVER_PULSES_PER_REV
};

static const int32_t axis_travel_um_per_rev[AXIS_COUNT] =
{
    [AXIS_X] = X_TRAVEL_UM_PER_REV,
    [AXIS_Y] = Y_TRAVEL_UM_PER_REV,
    [AXIS_Z] = Z_TRAVEL_UM_PER_REV
};

static const int32_t axis_home_offset_um[AXIS_COUNT] =
{
    [AXIS_X] = X_HOME_OFFSET_UM,
    [AXIS_Y] = Y_HOME_OFFSET_UM,
    [AXIS_Z] = Z_HOME_OFFSET_UM
};

void axis_init(void){
	for (int i = 0; i < AXIS_COUNT; i++){
		axes[i] = axis_t_constructor(0,
                                      0,
                                      0);
	}
}

uint8_t axis_check(axis_id_t axis){
    return (axis >= 0 && axis < AXIS_COUNT);
}

void axis_set_pos(axis_id_t axis, int32_t p_steps){
     if (axis_check(axis)) { axes[axis].pos_steps = p_steps; }
     else { return;}
}


void axis_set_target( axis_id_t axis, int32_t   t_steps){
    if (axis_check(axis)){axes[axis].target_steps = t_steps;}
    else {return;}
 }

void axis_set_is_homed(axis_id_t axis, uint8_t is_h){
    if (axis_check(axis)){
        axes[axis].is_homed = is_h; }
    else{return;}
}

int32_t axis_get_pos(axis_id_t axis) {
	if (axis_check(axis)){
         return axes[axis].pos_steps;
    }
	else{ return 0; }

}

int32_t axis_get_target( axis_id_t axis) {
	if (axis_check(axis)) {
		 return axes[axis].target_steps;
    }else { return 0 ; }
}

uint8_t axis_get_is_homed( axis_id_t axis) {
	 if (axis_check(axis)){
        return axes[axis].is_homed;
	 } else  { return 0; }

}

uint8_t axis_all_is_homed(void){
	uint8_t h_count = 0;
    for (int i = 0; i < AXIS_COUNT; i++) {
    	h_count += axis_get_is_homed((axis_id_t)i);
    }
    if ( h_count == AXIS_COUNT) return 1;
    else return 0;
}

int32_t axis_get_config_home_offset(axis_id_t axis)
{
    if (!axis_check(axis))
    {
        return 0;
    }

    return axis_um_to_steps(axis, axis_home_offset_um[axis]);
}

int32_t axis_um_to_steps(axis_id_t axis, int32_t um) {
    int64_t numerator;
    int64_t denominator;

    if (axis >= AXIS_COUNT || axis_travel_um_per_rev[axis] == 0) return 0;

    numerator = (int64_t)um * (int64_t)axis_driver_pulses_per_rev[axis];
    denominator = (int64_t)axis_travel_um_per_rev[axis];

    if (numerator >= 0)
    {
        return (int32_t)((numerator + (denominator / 2)) / denominator);
    }

    return (int32_t)((numerator - (denominator / 2)) / denominator);
}

int32_t axis_steps_to_um(axis_id_t axis, int32_t steps){
    int64_t numerator;
    int64_t denominator;

    if  (axis >= AXIS_COUNT || axis_driver_pulses_per_rev[axis] == 0)  return 0;

    numerator = (int64_t)steps * (int64_t)axis_travel_um_per_rev[axis];
    denominator = (int64_t)axis_driver_pulses_per_rev[axis];

    if (numerator >= 0)
    {
        return (int32_t)((numerator + (denominator / 2)) / denominator);
    }

    return (int32_t)((numerator - (denominator / 2)) / denominator);
}
