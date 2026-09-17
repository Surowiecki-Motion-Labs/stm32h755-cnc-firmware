#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "main.h"
#include "config_cm7.h"

static char g_logger_buf[LOGGER_BUF_SIZE];
static size_t g_logger_len = 0;

static uint8_t g_logger_enabled = 0;
static uint32_t g_logger_deadline = 0;

static void logger_update_state(void)
{
    if (g_logger_enabled)
    {
        uint32_t now = HAL_GetTick();

        if ((int32_t)(now - g_logger_deadline) >= 0)
        {
            g_logger_enabled = 0;
        }
    }
}

void logger_init(void)
{
    g_logger_buf[0] = '\0';
    g_logger_len = 0;
    g_logger_enabled = 0;
    g_logger_deadline = 0;
}

void logger_clear(void)
{
    g_logger_buf[0] = '\0';
    g_logger_len = 0;
}

void logger_arm_ms(uint32_t duration_ms)
{
    g_logger_enabled = 1;
    g_logger_deadline = HAL_GetTick() + duration_ms;
}

void logger_puts(const char *text)
{
    size_t len;
    size_t free_space;

    logger_update_state();

    if (!g_logger_enabled)
    {
        return;
    }

    if (text == 0)
    {
        return;
    }

    len = strlen(text);

    if (g_logger_len >= (LOGGER_BUF_SIZE - 1))
    {
        return;
    }

    free_space = (LOGGER_BUF_SIZE - 1) - g_logger_len;

    if (len > free_space)
    {
        len = free_space;
    }

    memcpy(&g_logger_buf[g_logger_len], text, len);
    g_logger_len += len;
    g_logger_buf[g_logger_len] = '\0';
}

void logger_printf(const char *fmt, ...)
{
    char temp[LOGGER_TEMP_SIZE];
    va_list args;
    int written;

    logger_update_state();

    if (!g_logger_enabled)
    {
        return;
    }

    if (fmt == 0)
    {
        return;
    }

    va_start(args, fmt);
    written = vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);

    if (written <= 0)
    {
        return;
    }

    logger_puts(temp);
}

const char *logger_get_buffer(void)
{
    return g_logger_buf;
}
