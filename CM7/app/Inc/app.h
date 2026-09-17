#ifndef APP_CM7_CORE_H
#define APP_CM7_CORE_H

#include <stdint.h>
#include "machine_state.h"
#include "gcode_parser.h"
#include "../Inc/ipc_shared.h"

typedef enum
{
    APP_RES_OK = 0,
    APP_RES_PARSE_ERR,
    APP_RES_BUILD_ERR,
    APP_RES_IPC_ERR,
    APP_RES_STATE_ERR,
    APP_RES_FULL_ERR
} app_res_t;

typedef enum
{
    APP_ERR_NONE = 0,
    APP_ERR_PARSE,
    APP_ERR_NO_FEED,
    APP_ERR_NO_SP,
    APP_ERR_UNSUPP,
    APP_ERR_NULL,
    APP_ERR_IPC,
    APP_ERR_BUILD,
    APP_ERR_STATE,
    APP_ERR_FULL
} app_err_t;

typedef struct
{
    app_res_t res;
    app_err_t err;
    uint8_t wait;
} app_proc_res_t;

typedef enum
{
    APP_PM_IDLE = 0,
    APP_PM_RUNNING,
    APP_PM_PAUSED,
    APP_PM_ERROR
} app_pm_state_t;

typedef struct
{
    m_state_t cm7_state;
    m_state_t add_state;
    g_cmd_t parsed_cmd;
    ipc_command_t ipc_cmd;
    uint8_t waiting;
    uint8_t motion_started;
    uint8_t program_end_pending;
} app_t;

static inline app_t app_t_constructor(m_state_t input_cm7_state,
                                      m_state_t input_add_state,
                                      g_cmd_t input_parsed_cmd,
                                      ipc_command_t input_ipc_cmd,
                                      uint8_t input_waiting,
                                      uint8_t input_motion_started,
                                      uint8_t input_program_end_pending)
{
    app_t output_app;
    output_app.cm7_state = input_cm7_state;
    output_app.add_state = input_add_state;
    output_app.parsed_cmd = input_parsed_cmd;
    output_app.ipc_cmd = input_ipc_cmd;
    output_app.waiting = input_waiting;
    output_app.motion_started = input_motion_started;
    output_app.program_end_pending = input_program_end_pending;
    return output_app;
}

static inline app_t app_t_default(void)
{
    return app_t_constructor((m_state_t){0},
                             (m_state_t){0},
                             (g_cmd_t){0},
                             (ipc_command_t){0},
                             0, 0, 0);
}

void app_init(void);
void app_run(void);

app_proc_res_t app_process_line(const char *line);
app_proc_res_t app_add_line(const char *line);
app_proc_res_t app_clear_program(void);
app_proc_res_t app_run_program(void);
app_proc_res_t app_pause_program(void);
app_proc_res_t app_resume_program(void);
app_proc_res_t app_stop_program(void);
app_proc_res_t app_set_feed_override(uint16_t percent);
app_proc_res_t app_set_spindle_override(uint16_t percent);

app_pm_state_t app_get_program_state(void);
uint16_t app_get_program_count(void);
uint32_t app_get_program_index(void);
uint16_t app_get_program_free(void);

#endif /* APP_CM7_CORE_H */
