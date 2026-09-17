#ifndef IPC_CM4_H
#define IPC_CM4_H

#include <stdint.h>
#include "ipc_shared.h"

#include "../../rt_status/Inc/rt_status.h"

#ifdef __cplusplus
extern "C" {
#endif

void ipc_cm4_init(void);
void ipc_cm4_poll(void);

void ipc_cm4_push_status(const rt_status_t *stat);

uint8_t ipc_cm4_pop_command(ipc_command_t *com);

#ifdef __cplusplus
}
#endif

#endif /* IPC_CM4_H */
