#include "gcode_parser.h"
#include "motion_cmd_builder.h"
#include "machine_state.h"
#include "ipc_cm7.h"
#include "program_manager.h"
#include "logger.h"
#include "config.h"


#include "axis_t.h"

#include <string.h>
#include "../../../CM4/rt_status/Inc/rt_status.h"
#include "../Inc/app.h"

static app_t global_app;

static app_proc_res_t app_ok(void)
{
    app_proc_res_t out;
    out.res = APP_RES_OK;
    out.err = APP_ERR_NONE;
    out.wait = 0;
    return out;
}

static app_proc_res_t app_make_err(app_res_t res, app_err_t err)
{
    app_proc_res_t out;
    out.res = res;
    out.err = err;
    out.wait = 0;
    return out;
}

static app_proc_res_t app_process_line_with_state(const char *line, m_state_t *state)
{
    if (line == 0 || state == 0)
    {
        return app_make_err(APP_RES_BUILD_ERR, APP_ERR_NULL);
    }

    const rt_status_t *st = ipc_cm7_get_status();

    if (st != 0 && state == &global_app.cm7_state)
    {
        m_state_set_pos(state,
                        st->position_um[AXIS_X],
                        st->position_um[AXIS_Y],
                        st->position_um[AXIS_Z]);
    }

    memset(&global_app.parsed_cmd, 0, sizeof(global_app.parsed_cmd));

    if (parse_line(line, &global_app.parsed_cmd) != PARSE_OK)
    {
        return app_make_err(APP_RES_PARSE_ERR, APP_ERR_PARSE);
    }

    cmd_build_error_t build_err = CMD_B_ERR_NONE;
    global_app.ipc_cmd = ipc_command_t_default();

    cmd_build_result_t build_res = motion_cmd_build(
        &global_app.parsed_cmd,
        state,
        &global_app.ipc_cmd,
        &build_err
    );

    if (build_res == CMD_BUILD_ERR)
    {
        switch (build_err)
        {
            case CMD_B_ERR_NO_FEED:
                return app_make_err(APP_RES_BUILD_ERR, APP_ERR_NO_FEED);
            case CMD_B_ERR_NO_SP:
                return app_make_err(APP_RES_BUILD_ERR, APP_ERR_NO_SP);
            case CMD_B_ERR_UNSUPP:
                return app_make_err(APP_RES_BUILD_ERR, APP_ERR_UNSUPP);
            case CMD_B_ERR_NULL:
                return app_make_err(APP_RES_BUILD_ERR, APP_ERR_NULL);
            default:
                return app_make_err(APP_RES_BUILD_ERR, APP_ERR_BUILD);
        }
    }

    app_proc_res_t out = app_ok();

    if (build_res == CMD_WITH_IPC && state == &global_app.cm7_state)
    {
        if (!ipc_cm7_send_command(&global_app.ipc_cmd))
        {
            return app_make_err(APP_RES_IPC_ERR, APP_ERR_IPC);
        }

        if (global_app.ipc_cmd.type == IPC_CMD_START_SEGMENT || global_app.ipc_cmd.type == IPC_CMD_HOME)
        {
            out.wait = 1;
        }
        else
        {
            out.wait = 0;
        }
    }

    return out;
}

static app_proc_res_t app_validate_add_line(const char *line)
{
    return app_process_line_with_state(line, &global_app.add_state);
}

void app_init(void)
{
	global_app = app_t_default();

	m_state_init(&global_app.cm7_state);
	m_state_init(&global_app.add_state);
	ipc_cm7_init();
	pm_init();
	logger_init();
}

app_proc_res_t app_process_line(const char *line)
{
    return app_process_line_with_state(line, &global_app.cm7_state);
}

app_proc_res_t app_add_line(const char *line)
{
    app_proc_res_t res = pm_add_line(line, app_validate_add_line);

    logger_printf("[CM7][app_add_line] line=%s res=%u err=%u count=%u\r\n",
                  (line ? line : "NULL"),
                  (unsigned)res.res,
                  (unsigned)res.err,
                  (unsigned)pm_get_count());

    return res;
}

app_proc_res_t app_clear_program(void)
{
    app_proc_res_t res = pm_clear();

    if (res.res == APP_RES_OK)
    {
        m_state_init(&global_app.cm7_state);
        m_state_init(&global_app.add_state);

        global_app.waiting = 0;
        global_app.motion_started = 0;
        global_app.program_end_pending = 0;
    }

    logger_printf("[CM7][app_clear_program] res=%u err=%u\r\n",
                  (unsigned)res.res,
                  (unsigned)res.err);

    return res;
}

app_proc_res_t app_run_program(void)
{
    app_proc_res_t res = pm_run();

    if (res.res == APP_RES_OK)
    {
        m_state_init(&global_app.cm7_state);
        m_state_init(&global_app.add_state);

        const rt_status_t *st = ipc_cm7_get_status();
        if (st != 0)
        {
        	m_state_set_pos(&global_app.cm7_state,
        	                st->position_um[AXIS_X],
        	                st->position_um[AXIS_Y],
        	                st->position_um[AXIS_Z]);

        	m_state_set_pos(&global_app.add_state,
        	                st->position_um[AXIS_X],
        	                st->position_um[AXIS_Y],
        	                st->position_um[AXIS_Z]);
        }

        global_app.waiting = 0;
        global_app.motion_started = 0;
        global_app.program_end_pending = 0;
    }

    logger_printf("[CM7][app_run_program] res=%u err=%u count=%u\r\n",
                  (unsigned)res.res,
                  (unsigned)res.err,
                  (unsigned)pm_get_count());

    return res;
}
app_proc_res_t app_pause_program(void)
{
    app_proc_res_t pm_res = pm_pause();
    if (pm_res.res != APP_RES_OK)
    {
        return pm_res;
    }

    ipc_command_t hold_cmd = ipc_command_t_default();
    hold_cmd.type = IPC_CMD_HOLD;

    if (!ipc_cm7_send_command(&hold_cmd))
    {
        return app_make_err(APP_RES_IPC_ERR, APP_ERR_IPC);
    }

    logger_printf("[CM7][app_pause_program] waiting=%u motion_started=%u end_pending=%u\r\n",
                  (unsigned)global_app.waiting,
                  (unsigned)global_app.motion_started,
                  (unsigned)global_app.program_end_pending);

    return pm_res;
}

app_proc_res_t app_resume_program(void)
{
    app_proc_res_t pm_res = pm_resume();
    if (pm_res.res != APP_RES_OK)
    {
        return pm_res;
    }

    global_app.motion_started = 0;

    ipc_command_t resume_cmd = ipc_command_t_default();
    resume_cmd.type = IPC_CMD_RESUME;

    if (!ipc_cm7_send_command(&resume_cmd))
    {
        return app_make_err(APP_RES_IPC_ERR, APP_ERR_IPC);
    }

    logger_printf("[CM7][app_resume_program] waiting=%u motion_started=%u end_pending=%u\r\n",
                  (unsigned)global_app.waiting,
                  (unsigned)global_app.motion_started,
                  (unsigned)global_app.program_end_pending);

    return pm_res;
}
app_proc_res_t app_stop_program(void)
{
    ipc_command_t stop_cmd = ipc_command_t_default();
    stop_cmd.type = IPC_CMD_STOP;

    if (!ipc_cm7_send_command(&stop_cmd))
    {
        return app_make_err(APP_RES_IPC_ERR, APP_ERR_IPC);
    }

    global_app.waiting = 0;
    global_app.motion_started = 0;
    global_app.program_end_pending = 0;

    app_proc_res_t pm_res = pm_stop();

    if (pm_res.res == APP_RES_OK)
    {
        m_state_init(&global_app.cm7_state);
        m_state_init(&global_app.add_state);
    }

    logger_printf("[CM7][app_stop_program] res=%u err=%u\r\n",
                  (unsigned)pm_res.res,
                  (unsigned)pm_res.err);

    return pm_res;
}
app_proc_res_t app_set_feed_override(uint16_t percent)
{
    if (percent < FEED_OVERRIDE_MIN) { percent = FEED_OVERRIDE_MIN; }
    if (percent > FEED_OVERRIDE_MAX) { percent = FEED_OVERRIDE_MAX; }

    ipc_command_t cmd = ipc_command_t_default();
    cmd.type = IPC_CMD_SET_FEED_OVERRIDE;
    cmd.feed_override.percent = percent;

    if (!ipc_cm7_send_command(&cmd))
    {
        return app_make_err(APP_RES_IPC_ERR, APP_ERR_IPC);
    }

    return app_ok();
}

app_proc_res_t app_set_spindle_override(uint16_t percent)
{
    if (percent < SPINDLE_OVERRIDE_MIN) { percent = SPINDLE_OVERRIDE_MIN; }
    if (percent > SPINDLE_OVERRIDE_MAX) { percent = SPINDLE_OVERRIDE_MAX; }

    ipc_command_t cmd = ipc_command_t_default();
    cmd.type = IPC_CMD_SET_SPINDLE_OVERRIDE;
    cmd.spindle_override.percent = percent;

    if (!ipc_cm7_send_command(&cmd))
    {
        return app_make_err(APP_RES_IPC_ERR, APP_ERR_IPC);
    }

    return app_ok();
}



app_pm_state_t app_get_program_state(void)
{
    return pm_get_state();
}

uint16_t app_get_program_count(void)
{
    return pm_get_count();
}

uint32_t app_get_program_index(void)
{
    return pm_get_index();
}

uint16_t app_get_program_free(void)
{
    return pm_get_free();
}

void app_run(void)
{
    const char *line;
    app_proc_res_t res;
    const rt_status_t *st;

    if (pm_get_state() != APP_PM_RUNNING)
    {
        return;
    }

    st = ipc_cm7_get_status();
    if (st == 0)
    {
        return;
    }

    if (global_app.waiting)
    {
        if (!global_app.motion_started)
        {
            if (st->busy)
            {
                global_app.motion_started = 1;
            }
            return;
        }

        if (st->busy)
        {
            return;
        }

        global_app.waiting = 0;
        global_app.motion_started = 0;
        pm_advance();
        return;
    }

    if (global_app.program_end_pending)
    {
        if (st->busy)
        {
            return;
        }

        global_app.program_end_pending = 0;
        pm_advance();
        return;
    }

    line = pm_get_current_line();

    if (line == 0)
    {
        global_app.program_end_pending = 1;
        return;
    }

    res = app_process_line(line);

    if (res.res != APP_RES_OK)
    {
        pm_set_error();
        global_app.waiting = 0;
        global_app.motion_started = 0;
        global_app.program_end_pending = 0;
        return;
    }

    if (res.wait)
    {
        global_app.waiting = 1;
        global_app.motion_started = 0;
    }
    else
    {
        pm_advance();

        if (pm_get_current_line() == 0)
        {
            global_app.program_end_pending = 1;
        }
    }
}
