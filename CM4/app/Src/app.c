#include "axis.h"
#include "safety.h"
#include "homing.h"
#include "motion_exec.h"
#include "ipc_cm4.h"
#include "stepgen.h"
#include "spindle.h"
#include "../Inc/app.h"
#include "../../rt_status/Inc/rt_status.h"
#include "config.h"

static void app_handle_ipc_command(void)
{
    ipc_command_t com;

    if (ipc_cm4_pop_command(&com))
    {
        switch (com.type)
        {
            case IPC_CMD_START_SEGMENT:
                m_exec_start(&com.segment);
                break;

            case IPC_CMD_STOP:
                m_exec_stop();
                spindle_stop();
                break;

            case IPC_CMD_HOLD:
                m_exec_hold();
                break;

            case IPC_CMD_RESUME:
                m_exec_resume();
                break;

            case IPC_CMD_HOME:
                if (com.home.home_x ||
                    com.home.home_y ||
                    com.home.home_z)
                {
                    homing_start_selected(
                        com.home.home_x,
                        com.home.home_y,
                        com.home.home_z
                    );
                }
                else
                {
                    homing_start_all();
                }
                break;

            case IPC_CMD_SPINDLE_SET:
                spindle_start(com.spindle.rpm, com.spindle.direction_cw);
                break;

            case IPC_CMD_SPINDLE_STOP:
                spindle_stop();
                break;

            case IPC_CMD_SET_FEED_OVERRIDE:
                sg_set_f_override(com.feed_override.percent);
                break;

            case IPC_CMD_SET_SPINDLE_OVERRIDE:
                spindle_set_override_percent(com.spindle_override.percent);
                break;

            case IPC_CMD_NONE:
            default:
                break;
        }
    }
}

void app_init(void)
{
    axis_init();
    rt_status_init();
    safety_init();
    homing_init();
    m_exec_init();
    spindle_init();
    ipc_cm4_init();
}

void app_run(void)
{
    const rt_status_t *cur;
    rt_status_t st;

    ipc_cm4_poll();
    app_handle_ipc_command();

    safety_run();
    homing_run();
    m_exec_run();

    rt_status_set_alarm(safety_get_alarm());
    rt_status_set_estop(safety_get_estop());
    rt_status_set_busy(homing_is_busy() || m_exec_is_busy());

    rt_status_update();

    cur = rt_status_get();
    st = *cur;

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        st.position_um[i]  = axis_steps_to_um((axis_id_t)i, st.position_steps[i]);
        st.target_um[i]    = axis_steps_to_um((axis_id_t)i, st.target_steps[i]);
        st.remaining_um[i] = axis_steps_to_um((axis_id_t)i, st.remaining_steps[i]);
    }

    ipc_cm4_push_status(&st);
}
