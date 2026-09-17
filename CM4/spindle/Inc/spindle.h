#ifndef SPINDLE_H
#define SPINDLE_H

#include <stdint.h>
#include "config.h"
#include "config_cm4.h"

typedef struct
{
    uint8_t on;
    int32_t base_rpm;
    int32_t effective_rpm;
    uint16_t override_percent;
    uint8_t dir_cw;
} spindle_t;

static inline spindle_t spindle_t_constructor(uint8_t input_on,
                                              int32_t input_base_rpm,
                                              int32_t input_effective_rpm,
                                              uint16_t input_override_percent,
                                              uint8_t input_dir_cw)
{
    spindle_t output_spindle;
    output_spindle.on = input_on;
    output_spindle.base_rpm = input_base_rpm;
    output_spindle.effective_rpm = input_effective_rpm;
    output_spindle.override_percent = input_override_percent;
    output_spindle.dir_cw = input_dir_cw;
    return output_spindle;
}

static inline spindle_t spindle_t_default(void)
{
    return spindle_t_constructor(0, 0, 0, SPINDLE_OVERRIDE_DEFAULT, SPINDLE_DEFAULT_DIR_CW);
}

void spindle_init(void);
void spindle_start(int32_t rpm, uint8_t direction_cw);
void spindle_stop(void);

void spindle_set_base_rpm(int32_t rpm);
void spindle_set_override_percent(uint16_t percent);

uint8_t spindle_is_on(void);

int32_t spindle_get_base_rpm(void);
int32_t spindle_get_effective_rpm(void);
uint16_t spindle_get_override_percent(void);

uint8_t spindle_get_direction_cw(void);

#endif /* SPINDLE_H */
