#ifndef GCODE_TYPES_H
#define GCODE_TYPES_H

#include <stdint.h>

typedef enum {
    G_MOTION_NONE = 0,
    G_MOTION_G0,
    G_MOTION_G1,
    G_MOTION_G28
} g_motion_type_t;

typedef enum {
    G_POS_NONE = 0,
    G_POS_G90,
    G_POS_G91
} g_pos_mode_type_t;

typedef enum {
    G_SPINDLE_NONE = 0,
    G_SPINDLE_M3,
    G_SPINDLE_M4,
    G_SPINDLE_M5
} g_spindle_type_t;

typedef enum {
    PARSE_OK = 0,
    PARSE_EMPTY,
    PARSE_MISSING_END,
    PARSE_UNSUPPORTED,
    PARSE_INVALID,
    PARSE_DUPLICATE_PARAM,
    PARSE_INVALID_NUMBER,
    PARSE_INVALID_PARAM_FOR_CMD
} g_parse_result_t;

/* ========================================================================== */
/* G_PARAM_TYPE_T                                                             */
/* ========================================================================== */
typedef struct {
    uint8_t has_x;
    uint8_t has_y;
    uint8_t has_z;
    uint8_t has_f;
    uint8_t has_s;

    uint8_t axis_select_x;
    uint8_t axis_select_y;
    uint8_t axis_select_z;

    int32_t x_um;
    int32_t y_um;
    int32_t z_um;

    int32_t f;
    int32_t s_rpm;
} g_param_type_t;

static inline g_param_type_t g_param_type_t_default(void)
{
    return (g_param_type_t){0};
}

typedef struct {
    uint8_t has_motion_code;
    uint8_t has_pos_mode_code;
    uint8_t has_spindle_code;

    g_motion_type_t motion_code;
    g_pos_mode_type_t pos_mode_code;
    g_spindle_type_t spindle_code;

    g_param_type_t param;
} g_cmd_t;

static inline g_cmd_t g_cmd_t_constructor(uint8_t h_motion, uint8_t h_pos, uint8_t h_spindle,
                                          g_motion_type_t motion,
                                          g_pos_mode_type_t pos,
                                          g_spindle_type_t spindle,
                                          g_param_type_t input_param)
{
    g_cmd_t output_cmd;
    output_cmd.has_motion_code = h_motion;
    output_cmd.has_pos_mode_code = h_pos;
    output_cmd.has_spindle_code = h_spindle;
    output_cmd.motion_code = motion;
    output_cmd.pos_mode_code = pos;
    output_cmd.spindle_code = spindle;
    output_cmd.param = input_param;
    return output_cmd;
}

static inline g_cmd_t g_cmd_t_default(void)
{
    return g_cmd_t_constructor(0, 0, 0,
                               G_MOTION_NONE,
                               G_POS_NONE,
                               G_SPINDLE_NONE,
                               g_param_type_t_default());
}

#endif /* GCODE_TYPES_H */
