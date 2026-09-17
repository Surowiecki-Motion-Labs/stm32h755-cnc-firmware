#include "machine_state.h"

void m_state_init(m_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    *state = m_state_t_default();
}

void m_state_set_pos_m(m_state_t *state, m_pos_mode_t mode)
{
    if (state == 0)
    {
        return;
    }

    state->pos_mode = mode;
}

m_pos_mode_t m_state_get_pos_m(const m_state_t *state)
{
    if (state == 0)
    {
        return M_POS_ABS;
    }

    return state->pos_mode;
}

void m_state_set_pos(m_state_t *state, int32_t x_um, int32_t y_um, int32_t z_um)
{
    if (state == 0)
    {
        return;
    }

    state->x_um = x_um;
    state->y_um = y_um;
    state->z_um = z_um;
}

void m_state_get_pos(const m_state_t *state, int32_t *x_um, int32_t *y_um, int32_t *z_um)
{
    if (state == 0)
    {
        return;
    }

    if (x_um != 0)
    {
        *x_um = state->x_um;
    }

    if (y_um != 0)
    {
        *y_um = state->y_um;
    }

    if (z_um != 0)
    {
        *z_um = state->z_um;
    }
}

void m_state_set_feed(m_state_t *state, int32_t feed)
{
    if (state == 0)
    {
        return;
    }

    state->feed = feed;
    state->feed_valid = 1;
}

uint8_t m_state_has_feed(const m_state_t *state)
{
    if (state == 0)
    {
        return 0;
    }

    return state->feed_valid;
}

int32_t m_state_get_feed(const m_state_t *state)
{
    if (state == 0)
    {
        return 0;
    }

    return state->feed;
}

void m_state_set_sp(m_state_t *state,
                    uint8_t spindle_on,
                    int32_t spindle_rpm,
                    m_spindle_dir_t spindle_dir)
{
    if (state == 0)
    {
        return;
    }

    state->spindle_on = spindle_on;
    state->spindle_rpm = spindle_rpm;
    state->spindle_rpm_valid = 1;
    state->spindle_dir = spindle_dir;
}

void m_state_set_sp_on(m_state_t *state, uint8_t spindle_on)
{
    if (state == 0)
    {
        return;
    }

    state->spindle_on = spindle_on;
}

uint8_t m_state_is_sp_on(const m_state_t *state)
{
    if (state == 0)
    {
        return 0;
    }

    return state->spindle_on;
}

uint8_t m_state_has_sp_rpm(const m_state_t *state)
{
    if (state == 0)
    {
        return 0;
    }

    return state->spindle_rpm_valid;
}

int32_t m_state_get_sp_rpm(const m_state_t *state)
{
    if (state == 0)
    {
        return 0;
    }

    return state->spindle_rpm;
}

m_spindle_dir_t m_state_get_sp_dir(const m_state_t *state)
{
    if (state == 0)
    {
        return M_SP_DIR_CW;
    }

    return state->spindle_dir;
}

void m_state_set_sp_rpm(m_state_t *state, int32_t spindle_rpm)
{
    if (state == 0)
    {
        return;
    }

    state->spindle_rpm = spindle_rpm;
    state->spindle_rpm_valid = 1;
}
