#ifndef RT_STATUS_H
#define RT_STATUS_H

#include "axis_t.h"
#include "config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int32_t position_steps[AXIS_COUNT];
    int32_t target_steps[AXIS_COUNT];
    int32_t remaining_steps[AXIS_COUNT];

    int32_t position_um[AXIS_COUNT];
    int32_t target_um[AXIS_COUNT];
    int32_t remaining_um[AXIS_COUNT];

    uint32_t base_feed;
    uint32_t effective_feed;
    uint16_t feed_override_percent;

    uint8_t spindle_on;
    int32_t spindle_base_rpm;
    int32_t spindle_effective_rpm;
    uint16_t spindle_override_percent;
    uint8_t spindle_dir_cw;

    uint8_t busy;
    uint8_t alarm;
    uint8_t estop;

    uint8_t homed[AXIS_COUNT];
    uint8_t all_homed;
} rt_status_t;


static inline rt_status_t rt_status_t_default(void)
{
    rt_status_t output_rt_status = (rt_status_t){0};

    output_rt_status.feed_override_percent = FEED_OVERRIDE_DEFAULT;
    output_rt_status.spindle_override_percent = SPINDLE_OVERRIDE_DEFAULT;

    return output_rt_status;
}

void rt_status_init(void);
void rt_status_update(void);

void rt_status_set_busy(uint8_t busy);
void rt_status_set_alarm(uint8_t alarm);
void rt_status_set_estop(uint8_t estop);

const rt_status_t* rt_status_get(void);

#ifdef __cplusplus
}
#endif

#endif /* RT_STATUS_H */
