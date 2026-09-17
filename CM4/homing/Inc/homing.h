#ifndef HOMING_H
#define HOMING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "axis_t.h"

typedef struct
{
    uint8_t busy;
    uint8_t req[AXIS_COUNT];
} homing_t;

static inline homing_t homing_t_constructor(uint8_t input_busy,
                                            const uint8_t input_req[AXIS_COUNT])
{
    homing_t output_homing;
    output_homing.busy = input_busy;

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        output_homing.req[i] = input_req[i];
    }

    return output_homing;
}


static inline homing_t homing_t_default(void)
{
    uint8_t zero_req[AXIS_COUNT] = {0};
    return homing_t_constructor(0, zero_req);
}


void homing_init(void);
void homing_run(void);

void homing_start_all(void);
void homing_start_axis(axis_id_t axis);
void homing_start_selected(uint8_t home_x, uint8_t home_y, uint8_t home_z);

uint8_t homing_is_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* HOMING_H */
