#include "ipc_cm4.h"
#include "ipc_mailbox.h"

void ipc_cm4_init(void)
{
}

void ipc_cm4_poll(void)
{
}

uint8_t ipc_cm4_pop_command(ipc_command_t *com)
{
    if (com == 0)
        return 0;

    if (!g_ipc_mailbox.cmd_pending)
        return 0;

    *com = g_ipc_mailbox.com;
    g_ipc_mailbox.cmd_pending = 0;
    g_ipc_mailbox.com.type = IPC_CMD_NONE;

    return 1;
}

void ipc_cm4_push_status(const rt_status_t *status)
{
    if (status == 0)
        return;

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        g_ipc_mailbox.status.position_steps[i] = status->position_steps[i];
        g_ipc_mailbox.status.target_steps[i] = status->target_steps[i];
        g_ipc_mailbox.status.remaining_steps[i] = status->remaining_steps[i];

        g_ipc_mailbox.status.position_um[i] = status->position_um[i];
        g_ipc_mailbox.status.target_um[i] = status->target_um[i];
        g_ipc_mailbox.status.remaining_um[i] = status->remaining_um[i];

        g_ipc_mailbox.status.homed[i] = status->homed[i];
    }

    g_ipc_mailbox.status.base_feed = status->base_feed;
    g_ipc_mailbox.status.effective_feed = status->effective_feed;
    g_ipc_mailbox.status.feed_override_percent = status->feed_override_percent;

    g_ipc_mailbox.status.spindle_on = status->spindle_on;
    g_ipc_mailbox.status.spindle_base_rpm = status->spindle_base_rpm;
    g_ipc_mailbox.status.spindle_effective_rpm = status->spindle_effective_rpm;
    g_ipc_mailbox.status.spindle_override_percent = status->spindle_override_percent;
    g_ipc_mailbox.status.spindle_dir_cw = status->spindle_dir_cw;

    g_ipc_mailbox.status.all_homed = status->all_homed;
    g_ipc_mailbox.status.busy = status->busy;
    g_ipc_mailbox.status.alarm = status->alarm;
    g_ipc_mailbox.status.estop = status->estop;
}
