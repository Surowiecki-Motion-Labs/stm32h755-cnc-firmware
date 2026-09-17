#ifndef MOTION_TYPES_H
#define MOTION_TYPES_H

#include <stdint.h>
#include "axis_t.h"

typedef struct
{
    int32_t target_steps[AXIS_COUNT];
    uint32_t feed;
} m_segment_t;

static inline m_segment_t m_segment_t_constructor(const int32_t input_target_steps[AXIS_COUNT],
                                                  uint32_t input_feed)
{
    m_segment_t output_m_segment_t;
    output_m_segment_t.feed = input_feed;

    for (int i = 0; i < AXIS_COUNT; i++) {
        output_m_segment_t.target_steps[i] = input_target_steps[i];
    }

    return output_m_segment_t;
}

static inline m_segment_t m_segment_t_default(void)
{
    int32_t zero_steps[AXIS_COUNT] = {0};
    return m_segment_t_constructor(zero_steps, 0);
}

#endif /* MOTION_TYPES_H */
