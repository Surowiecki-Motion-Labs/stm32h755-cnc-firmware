#ifndef IPC_SHARED_H
#define IPC_SHARED_H

#include <stdint.h>
#include "config.h"
#include "motion_types.h"

typedef enum
{
    IPC_CMD_NONE = 0,
    IPC_CMD_START_SEGMENT,
    IPC_CMD_STOP,
    IPC_CMD_HOLD,
    IPC_CMD_RESUME,
    IPC_CMD_SET_FEED_OVERRIDE,
    IPC_CMD_SET_SPINDLE_OVERRIDE,
    IPC_CMD_HOME,
    IPC_CMD_SPINDLE_SET,
    IPC_CMD_SPINDLE_STOP
} ipc_cmd_type_t;


typedef struct
{
    uint8_t home_x;
    uint8_t home_y;
    uint8_t home_z;
} ipc_home_t;

static inline ipc_home_t ipc_home_t_constructor(uint8_t input_home_x,
												uint8_t input_home_y,
												uint8_t input_home_z)
{
    ipc_home_t output_ipc_home_t;
    output_ipc_home_t.home_x = input_home_x;
    output_ipc_home_t.home_y = input_home_y;
    output_ipc_home_t.home_z = input_home_z;
    return output_ipc_home_t;
}

static inline ipc_home_t ipc_home_t_default(void)
{
    return ipc_home_t_constructor(0, 0, 0);
}


typedef struct
{
    int32_t rpm;
    uint8_t direction_cw;
} ipc_spindle_t;

static inline ipc_spindle_t ipc_spindle_t_constructor(int32_t input_rpm,
                                                      uint8_t input_direction_cw)
{
    ipc_spindle_t output_ipc_spindle_t;
    output_ipc_spindle_t.direction_cw = input_direction_cw;
    output_ipc_spindle_t.rpm = input_rpm;
    return output_ipc_spindle_t;
}

static inline ipc_spindle_t ipc_spindle_t_default(void)
{

    return ipc_spindle_t_constructor(0, 0);
}


typedef struct
{
    uint16_t percent;
} ipc_feed_override_t;

static inline ipc_feed_override_t ipc_feed_override_t_constructor(uint16_t input_percent)
{
    ipc_feed_override_t output_ipc_feed_override_t;
    output_ipc_feed_override_t.percent = input_percent;
    return output_ipc_feed_override_t;
}

static inline ipc_feed_override_t ipc_feed_override_t_default(void)
{
    return ipc_feed_override_t_constructor(FEED_OVERRIDE_DEFAULT);
}


typedef struct
{
    uint16_t percent;
} ipc_spindle_override_t;

static inline ipc_spindle_override_t ipc_spindle_override_t_constructor(uint16_t input_percent)
{
    ipc_spindle_override_t output_ipc_spindle_override_t;
    output_ipc_spindle_override_t.percent = input_percent;
    return output_ipc_spindle_override_t;
}

static inline ipc_spindle_override_t ipc_spindle_override_t_default(void)
{
    return ipc_spindle_override_t_constructor(SPINDLE_OVERRIDE_DEFAULT);
}

typedef struct
{
    ipc_cmd_type_t type;
    m_segment_t segment;
    ipc_home_t home;
    ipc_spindle_t spindle;
    ipc_feed_override_t feed_override;
    ipc_spindle_override_t spindle_override;
} ipc_command_t;

static inline ipc_command_t ipc_command_t_constructor(ipc_cmd_type_t input_type,
                                                      m_segment_t input_segment,
                                                      ipc_home_t input_home,
                                                      ipc_spindle_t input_spindle,
                                                      ipc_feed_override_t input_feed_override,
                                                      ipc_spindle_override_t input_spindle_override)
{
    ipc_command_t output_ipc_command_t;
    output_ipc_command_t.type = input_type;
    output_ipc_command_t.segment = input_segment;
    output_ipc_command_t.home = input_home;
    output_ipc_command_t.spindle = input_spindle;
    output_ipc_command_t.feed_override = input_feed_override;
    output_ipc_command_t.spindle_override = input_spindle_override;
    return output_ipc_command_t;
}

static inline ipc_command_t ipc_command_t_default(void)
{

    return ipc_command_t_constructor(IPC_CMD_NONE,
    									m_segment_t_default(),
                                     ipc_home_t_default(),
                                     ipc_spindle_t_default(),
                                     ipc_feed_override_t_default(),
                                     ipc_spindle_override_t_default());
}

#endif /* IPC_SHARED_H */
