#ifndef GCODE_PARSER_H
#define GCODE_PARSER_H

#include "gcode_types.h"

g_parse_result_t parse_line(const char *line, g_cmd_t *out_cmd);

#endif /* GCODE_PARSER_H */
