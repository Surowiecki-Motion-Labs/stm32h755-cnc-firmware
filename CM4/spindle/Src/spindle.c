#include "spindle.h"

static spindle_t global_spindle;

static int32_t spindle_apply_override(int32_t base_rpm, uint16_t percent)
{
    int32_t effective;

    if (base_rpm <= 0)
    {
        return 0;
    }

    if (percent < SPINDLE_OVERRIDE_MIN)
    {
        percent = SPINDLE_OVERRIDE_MIN;
    }

    if (percent > SPINDLE_OVERRIDE_MAX)
    {
        percent = SPINDLE_OVERRIDE_MAX;
    }

    effective = (int32_t)(((int64_t)base_rpm * (int64_t)percent) / (int64_t)PERCENT_SCALE);

    if (effective > SPINDLE_RPM_MAX)
    {
        effective = SPINDLE_RPM_MAX;
    }

    if (effective > 0 && effective < SPINDLE_RPM_MIN)
    {
        effective = SPINDLE_RPM_MIN;
    }

    return effective;
}

void spindle_init(void)
{
    global_spindle = spindle_t_default();
}

void spindle_start(int32_t rpm, uint8_t direction_cw)
{
    global_spindle.dir_cw = direction_cw ? 1U : 0U;
    global_spindle.on = 1;
    spindle_set_base_rpm(rpm);
}

void spindle_stop(void)
{
    global_spindle.on = 0;
    global_spindle.effective_rpm = 0;
}

void spindle_set_base_rpm(int32_t rpm)
{
    if (rpm < 0)
    {
        rpm = 0;
    }

    if (rpm > SPINDLE_RPM_MAX)
    {
        rpm = SPINDLE_RPM_MAX;
    }

    if (rpm > 0 && rpm < SPINDLE_RPM_MIN)
    {
        rpm = SPINDLE_RPM_MIN;
    }

    global_spindle.base_rpm = rpm;

    if (global_spindle.on)
    {
        global_spindle.effective_rpm = spindle_apply_override(global_spindle.base_rpm,
                                                              global_spindle.override_percent);
    }
    else
    {
        global_spindle.effective_rpm = 0;
    }
}

void spindle_set_override_percent(uint16_t percent)
{
    if (percent < SPINDLE_OVERRIDE_MIN)
    {
        percent = SPINDLE_OVERRIDE_MIN;
    }

    if (percent > SPINDLE_OVERRIDE_MAX)
    {
        percent = SPINDLE_OVERRIDE_MAX;
    }

    global_spindle.override_percent = percent;

    if (global_spindle.on)
    {
        global_spindle.effective_rpm = spindle_apply_override(global_spindle.base_rpm,
                                                              global_spindle.override_percent);
    }
    else
    {
        global_spindle.effective_rpm = 0;
    }
}

uint8_t spindle_is_on(void)
{
    return global_spindle.on;
}

int32_t spindle_get_base_rpm(void)
{
    return global_spindle.base_rpm;
}

int32_t spindle_get_effective_rpm(void)
{
    return global_spindle.effective_rpm;
}

uint16_t spindle_get_override_percent(void)
{
    return global_spindle.override_percent;
}

uint8_t spindle_get_direction_cw(void)
{
    return global_spindle.dir_cw;
}
