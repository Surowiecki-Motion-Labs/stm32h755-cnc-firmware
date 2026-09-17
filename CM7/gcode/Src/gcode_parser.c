#include "gcode_parser.h"
#include <ctype.h>

static uint8_t is_cmd_end(char c)
{
    return (c == '\0' || c == ';' || c == '\n' || c == '\r');
}

static uint8_t is_inline_space(char c)
{
    return (c == ' ' || c == '\t');
}

static uint8_t is_separator(char c)
{
    return (is_cmd_end(c) || is_inline_space(c));
}

static g_parse_result_t read_uint(const char *line, int *i, int *out_value)
{
    int value = 0;

    if (!isdigit((unsigned char)line[*i]))
    {
        return PARSE_INVALID_NUMBER;
    }

    while (isdigit((unsigned char)line[*i]))
    {
        value = value * 10 + (line[*i] - '0');
        (*i)++;
    }

    *out_value = value;
    return PARSE_OK;
}

static g_parse_result_t read_signed_int(const char *line, int *i, int32_t *out_value)
{
    int sign = 1;
    int32_t value = 0;

    if (line[*i] == '-')
    {
        sign = -1;
        (*i)++;
    }
    else if (line[*i] == '+')
    {
        (*i)++;
    }

    if (!isdigit((unsigned char)line[*i]))
    {
        return PARSE_INVALID_NUMBER;
    }

    while (isdigit((unsigned char)line[*i]))
    {
        value = value * 10 + (line[*i] - '0');
        (*i)++;
    }

    *out_value = value * sign;
    return PARSE_OK;
}

static g_parse_result_t set_gcode(g_cmd_t *cmd, int number)
{
    switch (number)
    {
        case 0:
            if (cmd->has_motion_code)
                return PARSE_UNSUPPORTED;
            cmd->has_motion_code = 1;
            cmd->motion_code = G_MOTION_G0;
            return PARSE_OK;

        case 1:
            if (cmd->has_motion_code)
                return PARSE_UNSUPPORTED;
            cmd->has_motion_code = 1;
            cmd->motion_code = G_MOTION_G1;
            return PARSE_OK;

        case 28:
            if (cmd->has_motion_code)
                return PARSE_UNSUPPORTED;
            cmd->has_motion_code = 1;
            cmd->motion_code = G_MOTION_G28;
            return PARSE_OK;

        case 90:
            if (cmd->has_pos_mode_code)
                return PARSE_UNSUPPORTED;
            cmd->has_pos_mode_code = 1;
            cmd->pos_mode_code = G_POS_G90;
            return PARSE_OK;

        case 91:
            if (cmd->has_pos_mode_code)
                return PARSE_UNSUPPORTED;
            cmd->has_pos_mode_code = 1;
            cmd->pos_mode_code = G_POS_G91;
            return PARSE_OK;

        default:
            return PARSE_UNSUPPORTED;
    }
}

static g_parse_result_t set_m_code(g_cmd_t *cmd, int number)
{
    switch (number)
    {
        case 3:
            if (cmd->has_spindle_code)
                return PARSE_UNSUPPORTED;
            cmd->has_spindle_code = 1;
            cmd->spindle_code = G_SPINDLE_M3;
            return PARSE_OK;

        case 4:
            if (cmd->has_spindle_code)
                return PARSE_UNSUPPORTED;
            cmd->has_spindle_code = 1;
            cmd->spindle_code = G_SPINDLE_M4;
            return PARSE_OK;

        case 5:
            if (cmd->has_spindle_code)
                return PARSE_UNSUPPORTED;
            cmd->has_spindle_code = 1;
            cmd->spindle_code = G_SPINDLE_M5;
            return PARSE_OK;

        default:
            return PARSE_UNSUPPORTED;
    }
}

static g_parse_result_t parse_xyz_param(g_cmd_t *cmd, char param_letter, const char *line, int *i)
{
    g_parse_result_t result;
    int32_t value = 0;
    char next_c;

    (*i)++;
    next_c = line[*i];

    if (is_separator(next_c))
    {
        switch (param_letter)
        {
            case 'X':
                if (cmd->param.has_x || cmd->param.axis_select_x)
                    return PARSE_DUPLICATE_PARAM;
                cmd->param.axis_select_x = 1;
                return PARSE_OK;

            case 'Y':
                if (cmd->param.has_y || cmd->param.axis_select_y)
                    return PARSE_DUPLICATE_PARAM;
                cmd->param.axis_select_y = 1;
                return PARSE_OK;

            case 'Z':
                if (cmd->param.has_z || cmd->param.axis_select_z)
                    return PARSE_DUPLICATE_PARAM;
                cmd->param.axis_select_z = 1;
                return PARSE_OK;

            default:
                return PARSE_INVALID;
        }
    }

    result = read_signed_int(line, i, &value);
    if (result != PARSE_OK)
    {
        return result;
    }

    switch (param_letter)
    {
        case 'X':
            if (cmd->param.has_x || cmd->param.axis_select_x)
                return PARSE_DUPLICATE_PARAM;
            cmd->param.has_x = 1;
            cmd->param.x_um = value;
            return PARSE_OK;

        case 'Y':
            if (cmd->param.has_y || cmd->param.axis_select_y)
                return PARSE_DUPLICATE_PARAM;
            cmd->param.has_y = 1;
            cmd->param.y_um = value;
            return PARSE_OK;

        case 'Z':
            if (cmd->param.has_z || cmd->param.axis_select_z)
                return PARSE_DUPLICATE_PARAM;
            cmd->param.has_z = 1;
            cmd->param.z_um = value;
            return PARSE_OK;

        default:
            return PARSE_INVALID;
    }
}

static g_parse_result_t parse_number_param(g_cmd_t *cmd, char param_letter, const char *line, int *i)
{
    g_parse_result_t result;
    int32_t value = 0;

    (*i)++;

    result = read_signed_int(line, i, &value);
    if (result != PARSE_OK)
    {
        return result;
    }

    switch (param_letter)
    {
        case 'F':
            if (cmd->param.has_f)
                return PARSE_DUPLICATE_PARAM;
            cmd->param.has_f = 1;
            cmd->param.f = value;
            return PARSE_OK;

        case 'S':
            if (cmd->param.has_s)
                return PARSE_DUPLICATE_PARAM;
            cmd->param.has_s = 1;
            cmd->param.s_rpm = value;
            return PARSE_OK;

        default:
            return PARSE_INVALID;
    }
}

static g_parse_result_t gcode_parse_param(g_cmd_t *cmd, char param_letter, const char *line, int *i)
{
    switch (param_letter)
    {
        case 'X':
        case 'Y':
        case 'Z':
            return parse_xyz_param(cmd, param_letter, line, i);

        case 'F':
        case 'S':
            return parse_number_param(cmd, param_letter, line, i);

        default:
            return PARSE_INVALID;
    }
}

static g_parse_result_t validate_cmd(const g_cmd_t *cmd)
{
	if (!cmd->has_motion_code &&
	    !cmd->has_pos_mode_code &&
	    !cmd->has_spindle_code &&
	    !cmd->param.has_s)
	{
	    return PARSE_EMPTY;
	}

    if (cmd->has_motion_code)
    {
        switch (cmd->motion_code)
        {
            case G_MOTION_G0:
            case G_MOTION_G1:
                if (cmd->param.has_s ||
                    cmd->param.axis_select_x ||
                    cmd->param.axis_select_y ||
                    cmd->param.axis_select_z)
                {
                    return PARSE_INVALID_PARAM_FOR_CMD;
                }
                break;

            case G_MOTION_G28:
                if (cmd->param.has_x ||
                    cmd->param.has_y ||
                    cmd->param.has_z ||
                    cmd->param.has_f ||
                    cmd->param.has_s)
                {
                    return PARSE_INVALID_PARAM_FOR_CMD;
                }
                break;

            default:
                return PARSE_INVALID;
        }
    }

    if (cmd->has_pos_mode_code)
    {
        if (cmd->param.axis_select_x ||
            cmd->param.axis_select_y ||
            cmd->param.axis_select_z ||
            cmd->param.has_s)
        {
            return PARSE_INVALID_PARAM_FOR_CMD;
        }
    }

    if (cmd->has_spindle_code)
    {
        switch (cmd->spindle_code)
        {
            case G_SPINDLE_M3:
            case G_SPINDLE_M4:
                if (cmd->param.axis_select_x ||
                    cmd->param.axis_select_y ||
                    cmd->param.axis_select_z)
                {
                    return PARSE_INVALID_PARAM_FOR_CMD;
                }
                break;

            case G_SPINDLE_M5:
                if (cmd->param.has_s)
                {
                    return PARSE_INVALID_PARAM_FOR_CMD;
                }
                break;

            default:
                return PARSE_INVALID;
        }
    }

    if (!cmd->has_motion_code)
    {
        if (cmd->param.has_x ||
            cmd->param.has_y ||
            cmd->param.has_z ||
            cmd->param.has_f ||
            cmd->param.axis_select_x ||
            cmd->param.axis_select_y ||
            cmd->param.axis_select_z)
        {
            return PARSE_INVALID_PARAM_FOR_CMD;
        }
    }


    return PARSE_OK;
}

g_parse_result_t parse_line(const char *line, g_cmd_t *out_cmd)
{
    g_cmd_t cmd = g_cmd_t_default();
    int i = 0;
    g_parse_result_t result;

    if (line == 0 || out_cmd == 0)
    {
        return PARSE_INVALID;
    }


    while (!is_cmd_end(line[i]))
    {
        char c = (char)toupper((unsigned char)line[i]);
        int number = 0;

        if (is_inline_space(c))
        {
            i++;
            continue;
        }

        if (c == 'G')
        {
            i++;

            result = read_uint(line, &i, &number);
            if (result != PARSE_OK)
            {
                return result;
            }

            result = set_gcode(&cmd, number);
            if (result != PARSE_OK)
            {
                return result;
            }

            continue;
        }

        if (c == 'M')
        {
            i++;

            result = read_uint(line, &i, &number);
            if (result != PARSE_OK)
            {
                return result;
            }

            result = set_m_code(&cmd, number);
            if (result != PARSE_OK)
            {
                return result;
            }

            continue;
        }

        if (c == 'X' || c == 'Y' || c == 'Z' ||
            c == 'F' || c == 'S')
        {
            result = gcode_parse_param(&cmd, c, line, &i);
            if (result != PARSE_OK)
            {
                return result;
            }

            continue;
        }

        return PARSE_INVALID;
    }

    if (line[i] != ';')
    {
        return PARSE_MISSING_END;
    }

    result = validate_cmd(&cmd);
    if (result != PARSE_OK)
    {
        return result;
    }

    *out_cmd = cmd;
    return PARSE_OK;
}
