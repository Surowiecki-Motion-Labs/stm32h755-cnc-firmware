#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void logger_init(void);
void logger_clear(void);

void logger_arm_ms(uint32_t duration_ms);

void logger_puts(const char *text);
void logger_printf(const char *fmt, ...);

const char *logger_get_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
