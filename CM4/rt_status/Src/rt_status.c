#include "../../rt_status/Inc/rt_status.h"

#include "axis.h"
#include "stepgen.h"
#include "spindle.h"

static rt_status_t rt_status;

void rt_status_init(void)
{
	rt_status = rt_status_t_default();
}

void rt_status_update(void)
{
    for (int i = 0; i < AXIS_COUNT; i++)
    {
        rt_status.position_steps[i] = axis_get_pos((axis_id_t)i);
        rt_status.homed[i] = axis_get_is_homed((axis_id_t)i);

        rt_status.target_steps[i] = axis_get_target((axis_id_t)i);
        rt_status.remaining_steps[i] = sg_get_remaining_steps((axis_id_t)i);
    }

    rt_status.base_feed = sg_get_base_feed_hz();
    rt_status.effective_feed = sg_get_effective_feed_hz();
    rt_status.feed_override_percent = sg_get_f_override();

    rt_status.spindle_on = spindle_is_on();
    rt_status.spindle_base_rpm = spindle_get_base_rpm();
    rt_status.spindle_effective_rpm = spindle_get_effective_rpm();
    rt_status.spindle_override_percent = spindle_get_override_percent();
    rt_status.spindle_dir_cw = spindle_get_direction_cw();

    rt_status.all_homed = axis_all_is_homed();
}
void rt_status_set_busy(uint8_t busy)
{
    rt_status.busy = busy;
}

void rt_status_set_alarm(uint8_t alarm)
{
    rt_status.alarm = alarm;
}

void rt_status_set_estop(uint8_t estop)
{
    rt_status.estop = estop;
}

const rt_status_t* rt_status_get(void)
{
    return &rt_status;
}
