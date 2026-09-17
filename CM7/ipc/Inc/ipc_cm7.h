#ifndef IPC_CM7_H
#define IPC_CM7_H

#include <stdint.h>
#include "ipc_shared.h"
#include "ipc_mailbox.h"
#include "../../../CM4/rt_status/Inc/rt_status.h"

#ifdef __cplusplus
extern "C" {
#endif

void ipc_cm7_init(void);

uint8_t ipc_cm7_send_command(const ipc_command_t *command);
const rt_status_t* ipc_cm7_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* IPC_CM7_H */
