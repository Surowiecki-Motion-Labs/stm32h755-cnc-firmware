#ifndef MOTION_EXEC_H
#define MOTION_EXEC_H

#include <stdint.h>
#include "motion_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void m_exec_init(void);
void m_exec_run(void);

uint8_t m_exec_start(const m_segment_t *seg);
void m_exec_stop(void);

uint8_t m_exec_hold(void);
uint8_t m_exec_resume(void);

uint8_t m_exec_is_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_EXEC_H */
