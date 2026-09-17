#include "program_manager.h"
#include <string.h>

static program_manager_t global_pm;

static app_proc_res_t pm_ok(void)
{
    app_proc_res_t out;
    out.res = APP_RES_OK;
    out.err = APP_ERR_NONE;
    out.wait = 0;
    return out;
}

static app_proc_res_t pm_err(app_res_t res, app_err_t err)
{
    app_proc_res_t out;
    out.res = res;
    out.err = err;
    out.wait = 0;
    return out;
}

static void pm_reset_all(void)
{
    global_pm = program_manager_t_default();
}

void pm_init(void)
{
    pm_reset_all();
}

app_proc_res_t pm_add_line(const char *line, app_proc_res_t (*validator)(const char *))
{
    if (line == 0 || validator == 0)
    {
        return pm_err(APP_RES_BUILD_ERR, APP_ERR_NULL);
    }

    if (global_pm.count >= PM_MAX_LINES)
    {
        return pm_err(APP_RES_FULL_ERR, APP_ERR_FULL);
    }

    size_t len = strlen(line);
    if (len == 0 || len >= PM_LINE_LEN)
    {
        return pm_err(APP_RES_BUILD_ERR, APP_ERR_BUILD);
    }

    app_proc_res_t check = validator(line);
    if (check.res != APP_RES_OK)
    {
        return check;
    }

    strcpy(global_pm.lines[global_pm.write_index], line);
    global_pm.write_index = (uint16_t)((global_pm.write_index + 1U) % PM_MAX_LINES);
    global_pm.count++;

    return pm_ok();
}

app_proc_res_t pm_clear(void)
{
    if (global_pm.state == APP_PM_RUNNING || global_pm.state == APP_PM_PAUSED)
    {
        return pm_err(APP_RES_STATE_ERR, APP_ERR_STATE);
    }

    pm_reset_all();
    return pm_ok();
}

app_proc_res_t pm_run(void)
{
    if (global_pm.state != APP_PM_IDLE)
    {
        return pm_err(APP_RES_STATE_ERR, APP_ERR_STATE);
    }

    if (global_pm.count == 0)
    {
        return pm_err(APP_RES_STATE_ERR, APP_ERR_STATE);
    }

    global_pm.state = APP_PM_RUNNING;
    return pm_ok();
}

app_proc_res_t pm_pause(void)
{
    if (global_pm.state != APP_PM_RUNNING)
    {
        return pm_err(APP_RES_STATE_ERR, APP_ERR_STATE);
    }

    global_pm.state = APP_PM_PAUSED;
    return pm_ok();
}

app_proc_res_t pm_resume(void)
{
    if (global_pm.state != APP_PM_PAUSED)
    {
        return pm_err(APP_RES_STATE_ERR, APP_ERR_STATE);
    }

    global_pm.state = APP_PM_RUNNING;
    return pm_ok();
}

app_proc_res_t pm_stop(void)
{
    if (global_pm.state != APP_PM_RUNNING && global_pm.state != APP_PM_PAUSED)
    {
        return pm_err(APP_RES_STATE_ERR, APP_ERR_STATE);
    }

    pm_reset_all();
    return pm_ok();
}

app_pm_state_t pm_get_state(void)
{
    return global_pm.state;
}

uint16_t pm_get_count(void)
{
    return global_pm.count;
}

uint16_t pm_get_free(void)
{
    return (uint16_t)(PM_MAX_LINES - global_pm.count);
}

uint32_t pm_get_index(void)
{
    return global_pm.total_executed;
}

const char *pm_get_current_line(void)
{
    if (global_pm.state != APP_PM_RUNNING)
    {
        return 0;
    }

    if (global_pm.count == 0)
    {
        return 0;
    }

    return global_pm.lines[global_pm.read_index];
}

void pm_advance(void)
{
    if (global_pm.state != APP_PM_RUNNING)
    {
        return;
    }

    if (global_pm.count == 0)
    {
        global_pm.state = APP_PM_IDLE;
        return;
    }

    global_pm.read_index = (uint16_t)((global_pm.read_index + 1U) % PM_MAX_LINES);
    global_pm.count--;
    global_pm.total_executed++;

    if (global_pm.count == 0)
    {
        global_pm.state = APP_PM_IDLE;
    }
}

void pm_set_error(void)
{
    global_pm.state = APP_PM_ERROR;
}
