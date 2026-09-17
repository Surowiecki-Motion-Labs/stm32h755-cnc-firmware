#ifndef AXIS_T_H
#define AXIS_T_H

#ifdef __cplusplus
extern "C" {
#endif



#include <stdint.h>
#include "config.h"

typedef enum {
	AXIS_X = 0,
	AXIS_Y = 1,
	AXIS_Z = 2,
	AXIS_ID_COUNT = AXIS_COUNT
} axis_id_t;



typedef struct
{
	int32_t pos_steps;    
	int32_t target_steps;
	uint8_t is_homed;  
} axis_t ;

static inline axis_t axis_t_constructor(int32_t input_pos_steps,
                                        int32_t input_target_steps,
                                        uint8_t input_is_homed)
{
    axis_t output_axis_t;
    output_axis_t.pos_steps = input_pos_steps;
    output_axis_t.target_steps = input_target_steps;
    output_axis_t.is_homed = input_is_homed;

    return output_axis_t;
}

#ifdef __cplusplus
}
#endif

#endif /* AXIS_T_H */
