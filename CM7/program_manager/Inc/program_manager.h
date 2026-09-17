#ifndef PROGRAM_MANAGER_H
#define PROGRAM_MANAGER_H

#include <stdint.h>
#include "../../app/Inc/app.h"
#include "config_cm7.h"

#define PM_MAX_LINES    PROGRAM_MAX_LINES
#define PM_LINE_LEN     PROGRAM_LINE_LEN


typedef struct
{
    char lines[PM_MAX_LINES][PM_LINE_LEN];
    uint16_t read_index;
    uint16_t write_index;
    uint16_t count;
    uint32_t total_executed;
    app_pm_state_t state;
} program_manager_t;

static inline program_manager_t program_manager_t_constructor(
    uint16_t input_read_idx,
    uint16_t input_write_idx,
    uint16_t input_count,
    uint32_t input_total_executed,
    app_pm_state_t input_state)
{
    program_manager_t output_pm = (program_manager_t){0};
    output_pm.read_index = input_read_idx;
    output_pm.write_index = input_write_idx;
    output_pm.count = input_count;
    output_pm.total_executed = input_total_executed;
    output_pm.state = input_state;

    return output_pm;
}

static inline program_manager_t program_manager_t_default(void)
{
    return program_manager_t_constructor(0, 0, 0, 0, APP_PM_IDLE);
}


void pm_init(void);

app_proc_res_t pm_add_line(const char *line, app_proc_res_t (*validator)(const char *));
app_proc_res_t pm_clear(void);
app_proc_res_t pm_run(void);
app_proc_res_t pm_pause(void);
app_proc_res_t pm_resume(void);
app_proc_res_t pm_stop(void);

app_pm_state_t pm_get_state(void);
uint16_t pm_get_count(void);
uint16_t pm_get_free(void);
uint32_t pm_get_index(void);

const char *pm_get_current_line(void);
void pm_advance(void);
void pm_set_error(void);

#endif /* PROGRAM_MANAGER_H */
