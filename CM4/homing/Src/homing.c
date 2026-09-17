#include "homing.h"
#include "axis.h"
#include "safety.h"

static homing_t global_homing;

void homing_init(void)
{
    global_homing = homing_t_default();
}

void homing_start_axis(axis_id_t axis)
{
    if (!axis_check(axis) || safety_get_estop() || safety_get_alarm())
    {
        return;
    }

    global_homing.busy = 1;
    global_homing.req[axis] = 1;

    axis_set_is_homed(axis, 0);
}

void homing_start_all(void)
{
    if (safety_get_estop() || safety_get_alarm())
    {
        return;
    }

    global_homing.busy = 1;

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        global_homing.req[i] = 1;
        axis_set_is_homed((axis_id_t)i, 0);
    }
}

void homing_start_selected(uint8_t home_x, uint8_t home_y, uint8_t home_z)
{
    if (safety_get_estop() || safety_get_alarm())
    {
        return;
    }

    uint8_t selected[AXIS_COUNT] = {home_x, home_y, home_z};

    for (int axis = 0; axis < AXIS_COUNT; axis++)
    {
        if (selected[axis])
        {
            homing_start_axis((axis_id_t)axis);
        }
    }
}

void homing_run(void)
{
    if (!global_homing.busy)
    {
        return;
    }

    for (int ind = 0; ind < AXIS_COUNT; ind++)
    {
        if (global_homing.req[ind])
        {
            const axis_id_t axis = (axis_id_t)ind;
            const int32_t home_offset = axis_get_config_home_offset(axis);

            axis_set_pos(axis, home_offset);
            axis_set_target(axis, home_offset);
            axis_set_is_homed(axis, 1);

            global_homing.req[ind] = 0;
        }
    }

    uint8_t any_req = 0;
    for (int j = 0; j < AXIS_COUNT; j++)
    {
        if (global_homing.req[j])
        {
            any_req = 1;
            break;
        }
    }

    if (!any_req)
    {
        global_homing.busy = 0;
    }
}

uint8_t homing_is_busy(void)
{
    return global_homing.busy;
}
