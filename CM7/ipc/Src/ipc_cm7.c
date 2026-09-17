#include "ipc_cm7.h"
#include "ipc_mailbox.h"
#include "logger.h"

void ipc_cm7_init(void)
{
    g_ipc_mailbox.cmd_pending = 0;

    g_ipc_mailbox.com.type = IPC_CMD_NONE;
    g_ipc_mailbox.com.segment.feed = 0;

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        g_ipc_mailbox.com.segment.target_steps[i] = 0;

        g_ipc_mailbox.status.position_steps[i] = 0;
        g_ipc_mailbox.status.target_steps[i] = 0;
        g_ipc_mailbox.status.remaining_steps[i] = 0;

        g_ipc_mailbox.status.position_um[i] = 0;
        g_ipc_mailbox.status.target_um[i] = 0;
        g_ipc_mailbox.status.remaining_um[i] = 0;

        g_ipc_mailbox.status.homed[i] = 0;
    }

    g_ipc_mailbox.status.base_feed = 0;
    g_ipc_mailbox.status.effective_feed = 0;
    g_ipc_mailbox.status.feed_override_percent = 100;

    g_ipc_mailbox.status.spindle_on = 0;
    g_ipc_mailbox.status.spindle_base_rpm = 0;
    g_ipc_mailbox.status.spindle_effective_rpm = 0;
    g_ipc_mailbox.status.spindle_dir_cw = 1;

    g_ipc_mailbox.status.all_homed = 0;
    g_ipc_mailbox.status.busy = 0;
    g_ipc_mailbox.status.alarm = 0;
    g_ipc_mailbox.status.estop = 0;
}

uint8_t ipc_cm7_send_command(const ipc_command_t *com)
{
    if (com == 0)
    {
        logger_printf("[CM7][ipc_cm7_send_command] null command\r\n");
        return 0;
    }

    if (g_ipc_mailbox.cmd_pending)
    {
        logger_printf("[CM7][ipc_cm7_send_command] mailbox busy\r\n");
        return 0;
    }

    g_ipc_mailbox.com = *com;
    g_ipc_mailbox.cmd_pending = 1;

    logger_printf("[CM7][ipc_cm7_send_command] type=%u pending=1\r\n",
                  (unsigned)com->type);

    return 1;
}

const rt_status_t* ipc_cm7_get_status(void)
{
    return (const rt_status_t*)&g_ipc_mailbox.status;
}
