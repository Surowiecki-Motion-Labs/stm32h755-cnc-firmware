#include "safety.h"

static safety_t global_safety;

void safety_init(void)
{
    global_safety = safety_t_default();
}

void safety_run(void)
{
}

uint8_t safety_get_alarm(void)
{
    return global_safety.is_alarm;
}

uint8_t safety_get_estop(void)
{
    return global_safety.is_estop;
}
