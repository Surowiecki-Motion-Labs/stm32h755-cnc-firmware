#ifndef IPC_MAILBOX_H
#define IPC_MAILBOX_H

#include <stdint.h>
#include "ipc_shared.h"
#include "config.h"

#include "../../rt_status/Inc/rt_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    volatile uint32_t cmd_pending;
    ipc_command_t com;

    rt_status_t status;

} ipc_mailbox_t;

#if defined(CORE_CM7)
#define IPC_MAILBOX_ADDR   IPC_MAILBOX_ADDR_CM7

#elif defined(CORE_CM4)
#define IPC_MAILBOX_ADDR   IPC_MAILBOX_ADDR_CM4

#else
#error "IPC_mailbox : nieznany rdzen CORE_CM7 albo CORE_CM4."
#endif

#define g_ipc_mailbox (*(volatile ipc_mailbox_t *)IPC_MAILBOX_ADDR)

#ifdef __cplusplus
}
#endif

#endif /* IPC_MAILBOX_H */
