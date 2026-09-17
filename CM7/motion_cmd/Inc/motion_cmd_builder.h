#ifndef MOTION_CMD_BUILDER_H
#define MOTION_CMD_BUILDER_H

#include <stdint.h>
#include "gcode_types.h"
#include "machine_state.h"
#include "ipc_shared.h"


typedef enum {
    CMD_NO_IPC = 0,
     CMD_WITH_IPC,
    CMD_BUILD_ERR
}cmd_build_result_t;


typedef enum {
    CMD_B_ERR_NONE = 0,
    CMD_B_ERR_NULL,
    CMD_B_ERR_UNSUPP,
    CMD_B_ERR_NO_FEED,
    CMD_B_ERR_NO_SP
} cmd_build_error_t;

cmd_build_result_t motion_cmd_build(const  g_cmd_t *gcmd,
                                    m_state_t *state,
                                    ipc_command_t *out_cmd,
                                     cmd_build_error_t *out_error);

#endif /* MOTION_CMD_BUILDER_H */
