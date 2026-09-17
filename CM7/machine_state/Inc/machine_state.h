#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#include <stdint.h>

typedef enum {
    M_POS_ABS = 0,
    M_POS_INCR
} m_pos_mode_t;

typedef enum {
    M_SP_DIR_CW = 0,
    M_SP_DIR_CCW
} m_spindle_dir_t;

typedef struct {
    m_pos_mode_t pos_mode;

    int32_t x_um;
    int32_t y_um;
    int32_t z_um;

    int32_t feed;
    uint8_t feed_valid;

    uint8_t spindle_on;
    int32_t spindle_rpm;
    uint8_t spindle_rpm_valid;
    m_spindle_dir_t spindle_dir;
} m_state_t;


static inline m_state_t m_state_t_constructor(m_pos_mode_t mode,
                                              int32_t x, int32_t y, int32_t z,
                                              int32_t feed, uint8_t feed_v,
                                              uint8_t sp_on, int32_t sp_rpm, uint8_t sp_rpm_v,
                                              m_spindle_dir_t sp_dir)
{
    m_state_t output_state;
    output_state.pos_mode = mode;
    output_state.x_um = x;
    output_state.y_um = y;
    output_state.z_um = z;
    output_state.feed = feed;
    output_state.feed_valid = feed_v;
    output_state.spindle_on = sp_on;
    output_state.spindle_rpm = sp_rpm;
    output_state.spindle_rpm_valid = sp_rpm_v;
    output_state.spindle_dir = sp_dir;
    return output_state;
}

static inline m_state_t m_state_t_default(void)
{
    return m_state_t_constructor(M_POS_ABS,
                                 0, 0, 0,
                                 0, 0,
                                 0, 0, 0,
                                 M_SP_DIR_CW);
}


void m_state_init(m_state_t *state);

void m_state_set_pos_m(m_state_t *state, m_pos_mode_t mode);
m_pos_mode_t m_state_get_pos_m(const m_state_t *state);

void m_state_set_pos(m_state_t *state, int32_t x_um, int32_t y_um, int32_t z_um);
void m_state_get_pos(const m_state_t *state, int32_t *x_um, int32_t *y_um, int32_t *z_um);

void m_state_set_feed(m_state_t *state, int32_t feed);
uint8_t m_state_has_feed(const m_state_t *state);
int32_t m_state_get_feed(const m_state_t *state);

void m_state_set_sp(m_state_t *state,
                    uint8_t spindle_on,
                    int32_t spindle_rpm,
                    m_spindle_dir_t spindle_dir);
void m_state_set_sp_rpm(m_state_t *state, int32_t spindle_rpm);

void m_state_set_sp_on(m_state_t *state, uint8_t spindle_on);
uint8_t m_state_is_sp_on(const m_state_t *state);

uint8_t m_state_has_sp_rpm(const m_state_t *state);
int32_t m_state_get_sp_rpm(const m_state_t *state);
m_spindle_dir_t m_state_get_sp_dir(const m_state_t *state);

#endif /* MACHINE_STATE_H */
