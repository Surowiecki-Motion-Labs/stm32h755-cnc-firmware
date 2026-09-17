#ifndef SAFETY_H
#define SAFETY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t is_estop;
    uint8_t is_alarm;
} safety_t;

static inline safety_t safety_t_constructor(uint8_t input_is_estop, uint8_t input_is_alarm)
{
    safety_t output_safety;
    output_safety.is_estop = input_is_estop;
    output_safety.is_alarm = input_is_alarm;
    return output_safety;
}

static inline safety_t safety_t_default(void)
{
    return safety_t_constructor(0, 0);
}


void safety_init(void);
void safety_run(void);

uint8_t safety_get_alarm(void);
uint8_t safety_get_estop(void);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_H */
