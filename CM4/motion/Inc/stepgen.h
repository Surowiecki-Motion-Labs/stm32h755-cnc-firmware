#ifndef STEPGEN_H
#define STEPGEN_H

#include <stdint.h>
#include "axis_t.h"
#include "motion_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void sg_init(void);

uint8_t sg_start( const m_segment_t  *segment);

void sg_stop(void);

uint8_t sg_hold(void);
uint8_t sg_resume(void);

void sg_time_tick(void);
void sg_run_idle(void);

uint8_t  sg_is_busy(void);

int32_t  sg_get_remaining_steps(axis_id_t  axis);
void sg_set_f_override(uint16_t percent);
uint16_t sg_get_f_override(void);
uint32_t sg_get_base_feed_hz(void);
uint32_t sg_get_effective_feed_hz ( void);

#ifdef __cplusplus
}
#endif

#endif /* STEPGEN_H */
