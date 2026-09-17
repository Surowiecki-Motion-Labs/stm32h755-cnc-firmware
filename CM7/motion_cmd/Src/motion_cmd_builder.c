#include "motion_cmd_builder.h"
#include "axis_t.h"
#include "config.h"
#include "config_cm7.h"

#include <stdint.h>


static const int32_t axis_driver_pulses_per_rev[AXIS_COUNT] = {
    [AXIS_X] = X_DRIVER_PULSES_PER_REV,
    [AXIS_Y] = Y_DRIVER_PULSES_PER_REV,
    [AXIS_Z] = Z_DRIVER_PULSES_PER_REV
};

static const int32_t axis_travel_um_per_rev[AXIS_COUNT] = {
    [AXIS_X] = X_TRAVEL_UM_PER_REV,
    [AXIS_Y] = Y_TRAVEL_UM_PER_REV,
    [AXIS_Z] = Z_TRAVEL_UM_PER_REV
};

static int32_t um_to_steps(uint8_t axis, int32_t um)
{
    int64_t numerator;
    int64_t denominator;

    if (axis >= AXIS_COUNT || axis_travel_um_per_rev[axis] == 0)
    {
        return 0;
    }

    numerator = (int64_t)um * (int64_t)axis_driver_pulses_per_rev[axis];
    denominator = (int64_t)axis_travel_um_per_rev[axis];

    if (numerator >= 0)
    {
        return (int32_t)((numerator + (denominator / 2)) / denominator);
    }

    return (int32_t)((numerator - (denominator / 2)) / denominator);
}

static uint32_t feed_mm_min_to_steps_s(uint8_t axis, uint32_t feed_mm_min)
{
    uint64_t numerator;
    uint64_t denominator;
    uint64_t rounded;
    uint32_t steps_s;

    if (axis >= AXIS_COUNT || axis_travel_um_per_rev[axis] == 0)
    {
        return 1U;
    }

    numerator = (uint64_t)feed_mm_min *
                (uint64_t)UM_PER_MM *
                (uint64_t)axis_driver_pulses_per_rev[axis];

    denominator = (uint64_t)axis_travel_um_per_rev[axis] *
                  (uint64_t)SECONDS_PER_MINUTE;

    rounded = (numerator + (denominator / 2U)) / denominator;

    if (rounded > UINT32_MAX)
    {
        steps_s = UINT32_MAX;
    }
    else
    {
        steps_s = (uint32_t)rounded;
    }

    if (steps_s == 0U)
    {
        steps_s = 1U;
    }

    return steps_s;
}

static void ipc_cmd_clear(ipc_command_t *cmd)
{
    int i;

    cmd->type = IPC_CMD_NONE;

    for (i = 0; i < AXIS_COUNT; i++)
    {
        cmd->segment.target_steps[i] = 0;
    }

    cmd->segment.feed = 0;


    cmd->home.home_x = 0;
    cmd->home.home_y = 0;
    cmd->home.home_z = 0;

    cmd->spindle.rpm = 0;
    cmd->spindle.direction_cw = 0;
}

static void set_error(cmd_build_error_t *out_error, cmd_build_error_t error)
{
    if (out_error == 0)
    {
        return;
    }

    *out_error = error;
}

static m_pos_mode_t get_effective_pos_mode(const g_cmd_t *gcmd, const m_state_t *state)
{
    if (!gcmd->has_pos_mode_code)
    {
        return m_state_get_pos_m(state);
    }

    if (gcmd->pos_mode_code == G_POS_G90)
    {
        return M_POS_ABS;
    }
    else if (gcmd->pos_mode_code == G_POS_G91)
    {
        return M_POS_INCR;
    }

    return m_state_get_pos_m(state);
}

static void apply_pos_mode_if_present(const g_cmd_t *gcmd, m_state_t *state)
{
    if (!gcmd->has_pos_mode_code)
    {
        return;
    }

    if (gcmd->pos_mode_code == G_POS_G90)
    {
        m_state_set_pos_m(state, M_POS_ABS);
    }
    else if (gcmd->pos_mode_code == G_POS_G91)
    {
        m_state_set_pos_m(state, M_POS_INCR);
    }
}

static cmd_build_result_t build_position_mode_only(const g_cmd_t *gcmd,
                                                   m_state_t *state,
                                                   cmd_build_error_t *out_error)
{
    apply_pos_mode_if_present(gcmd, state);
    set_error(out_error, CMD_B_ERR_NONE);
    return CMD_NO_IPC;
}

static cmd_build_result_t build_linear_move(const g_cmd_t *gcmd,
                                            m_state_t *state,
                                            ipc_command_t *out_cmd,
                                            cmd_build_error_t *out_error)
{
    int32_t cur_x;
    int32_t cur_y;
    int32_t cur_z;

    int32_t dst_x;
    int32_t dst_y;
    int32_t dst_z;

    uint32_t feed_mm_min;
    uint32_t feed_steps_s;
    m_pos_mode_t pos_mode;

    pos_mode = get_effective_pos_mode(gcmd, state);

    m_state_get_pos(state, &cur_x, &cur_y, &cur_z);

    dst_x = cur_x;
    dst_y = cur_y;
    dst_z = cur_z;

    if (gcmd->motion_code == G_MOTION_G0)
    {
        feed_mm_min = RAPID_FEED_MM_MIN;
    }
    else
    {
        if (gcmd->param.has_f)
        {
            m_state_set_feed(state, gcmd->param.f);
            feed_mm_min = (uint32_t)gcmd->param.f;
        }
        else
        {
            if (!m_state_has_feed(state))
            {
                set_error(out_error, CMD_B_ERR_NO_FEED);
                return CMD_BUILD_ERR;
            }

            feed_mm_min = (uint32_t)m_state_get_feed(state);
        }
    }

    if (pos_mode == M_POS_ABS)
    {
        if (gcmd->param.has_x)
        {
            dst_x = gcmd->param.x_um;
        }

        if (gcmd->param.has_y)
        {
            dst_y = gcmd->param.y_um;
        }

        if (gcmd->param.has_z)
        {
            dst_z = gcmd->param.z_um;
        }
    }
    else
    {
        if (gcmd->param.has_x)
        {
            dst_x = cur_x + gcmd->param.x_um;
        }

        if (gcmd->param.has_y)
        {
            dst_y = cur_y + gcmd->param.y_um;
        }

        if (gcmd->param.has_z)
        {
            dst_z = cur_z + gcmd->param.z_um;
        }
    }

    if (dst_x == cur_x && dst_y == cur_y && dst_z == cur_z)
    {
        m_state_set_pos(state, dst_x, dst_y, dst_z);
        apply_pos_mode_if_present(gcmd, state);

        set_error(out_error, CMD_B_ERR_NONE);
        return CMD_NO_IPC;
    }

    feed_steps_s = feed_mm_min_to_steps_s(AXIS_X, feed_mm_min);

    out_cmd->type = IPC_CMD_START_SEGMENT;
    out_cmd->segment.feed = feed_steps_s;

    out_cmd->segment.target_steps[AXIS_X] = um_to_steps(AXIS_X, dst_x);
    out_cmd->segment.target_steps[AXIS_Y] = um_to_steps(AXIS_Y, dst_y);
    out_cmd->segment.target_steps[AXIS_Z] = um_to_steps(AXIS_Z, dst_z);

    m_state_set_pos(state, dst_x, dst_y, dst_z);
    apply_pos_mode_if_present(gcmd, state);

    set_error(out_error, CMD_B_ERR_NONE);
    return CMD_WITH_IPC;
}

static cmd_build_result_t build_home(const g_cmd_t *gcmd,
                                     ipc_command_t *out_cmd,
                                     cmd_build_error_t *out_error)
{
    out_cmd->type = IPC_CMD_HOME;

    out_cmd->home.home_x = gcmd->param.axis_select_x;
    out_cmd->home.home_y = gcmd->param.axis_select_y;
    out_cmd->home.home_z = gcmd->param.axis_select_z;

    set_error(out_error, CMD_B_ERR_NONE);
    return CMD_WITH_IPC;
}

static cmd_build_result_t build_spindle_set(const g_cmd_t *gcmd,
                                            m_state_t *state,
                                            ipc_command_t *out_cmd,
                                            cmd_build_error_t *out_error)
{
    int32_t rpm;
    m_spindle_dir_t dir;

    if (gcmd->param.has_s)
    {
        rpm = gcmd->param.s_rpm;
    }
    else
    {
        if (!m_state_has_sp_rpm(state))
        {
            set_error(out_error, CMD_B_ERR_NO_SP);
            return CMD_BUILD_ERR;
        }

        rpm = m_state_get_sp_rpm(state);
    }

    if (gcmd->spindle_code == G_SPINDLE_M3)
    {
        dir = M_SP_DIR_CW;
        out_cmd->spindle.direction_cw = 1;
    }
    else
    {
        dir = M_SP_DIR_CCW;
        out_cmd->spindle.direction_cw = 0;
    }

    out_cmd->type = IPC_CMD_SPINDLE_SET;
    out_cmd->spindle.rpm = rpm;

    m_state_set_sp(state, 1, rpm, dir);

    set_error(out_error, CMD_B_ERR_NONE);
    return CMD_WITH_IPC;
}



static cmd_build_result_t build_spindle_stop(m_state_t *state,
                                             ipc_command_t *out_cmd,
                                             cmd_build_error_t *out_error)
{
    out_cmd->type = IPC_CMD_SPINDLE_STOP;
    m_state_set_sp_on(state, 0);

    set_error(out_error, CMD_B_ERR_NONE);
    return CMD_WITH_IPC;
}

static cmd_build_result_t build_spindle_speed_only(const g_cmd_t *gcmd,
                                                   m_state_t *state,
                                                   ipc_command_t *out_cmd,
                                                   cmd_build_error_t *out_error)
{
    int32_t rpm;
    m_spindle_dir_t dir;

    if (!gcmd->param.has_s)
    {
        set_error(out_error, CMD_B_ERR_UNSUPP);
        return CMD_BUILD_ERR;
    }

    rpm = gcmd->param.s_rpm;
    m_state_set_sp_rpm(state, rpm);

    if (m_state_is_sp_on(state))
    {
        dir = m_state_get_sp_dir(state);

        out_cmd->type = IPC_CMD_SPINDLE_SET;
        out_cmd->spindle.rpm = rpm;
        out_cmd->spindle.direction_cw = (dir == M_SP_DIR_CW) ? 1U : 0U;

        set_error(out_error, CMD_B_ERR_NONE);
        return CMD_WITH_IPC;
    }

    set_error(out_error, CMD_B_ERR_NONE);
    return CMD_NO_IPC;
}

cmd_build_result_t motion_cmd_build(const g_cmd_t *gcmd,
                                    m_state_t *state,
                                    ipc_command_t *out_cmd,
                                    cmd_build_error_t *out_error)
{
    if (gcmd == 0 || state == 0 || out_cmd == 0)
    {
        set_error(out_error, CMD_B_ERR_NULL);
        return CMD_BUILD_ERR;
    }

    ipc_cmd_clear(out_cmd);
    set_error(out_error, CMD_B_ERR_NONE);

    if (gcmd->has_motion_code)
    {
        switch (gcmd->motion_code)
        {
            case G_MOTION_G0:
            case G_MOTION_G1:
                return build_linear_move(gcmd, state, out_cmd, out_error);

            case G_MOTION_G28:
                return build_home(gcmd, out_cmd, out_error);

            default:
                set_error(out_error, CMD_B_ERR_UNSUPP);
                return CMD_BUILD_ERR;
        }
    }

    if (gcmd->has_spindle_code)
    {
        switch (gcmd->spindle_code)
        {
            case G_SPINDLE_M3:
            case G_SPINDLE_M4:
                return build_spindle_set(gcmd, state, out_cmd, out_error);

            case G_SPINDLE_M5:
                return build_spindle_stop(state, out_cmd, out_error);

            default:
                set_error(out_error, CMD_B_ERR_UNSUPP);
                return CMD_BUILD_ERR;
        }
    }

    if (gcmd->has_pos_mode_code)
    {
        return build_position_mode_only(gcmd, state, out_error);
    }

    if (gcmd->param.has_s)
    {
        return build_spindle_speed_only(gcmd, state, out_cmd, out_error);
    }
    set_error(out_error, CMD_B_ERR_UNSUPP);
    return CMD_BUILD_ERR;

}
