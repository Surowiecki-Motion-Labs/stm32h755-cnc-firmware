#include "motion_exec.h"
#include "stepgen.h"
#include "safety.h"

static m_segment_t m_current_segment;
static uint8_t m_segment_valid = 0U;

void m_exec_init(void)
{
    m_segment_valid = 0U;
    sg_init();
}

uint8_t m_exec_start(const m_segment_t *segment)
{
    if (segment == 0)
    {
        return 0U;
    }

    if (safety_get_estop() || safety_get_alarm())
    {
        return 0U;
    }

    if (sg_is_busy())
    {
        return 0U;
    }

    m_current_segment = *segment;
    m_segment_valid = 1U;

    return sg_start(&m_current_segment);
}

void m_exec_stop(void)
{
    m_segment_valid = 0U;
    sg_stop();
}

uint8_t m_exec_hold(void)
{
    return sg_hold();
}

uint8_t m_exec_resume(void)
{
    return sg_resume();
}

void m_exec_run(void)
{
    sg_run_idle();

    if (!m_segment_valid)
    {
        return;
    }

    if (!sg_is_busy())
    {
        m_segment_valid = 0U;
    }
}

uint8_t m_exec_is_busy(void)
{
    return sg_is_busy();
}
