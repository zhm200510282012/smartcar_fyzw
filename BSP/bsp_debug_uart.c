#include "bsp_debug_uart.h"

static const char * volatile g_last_debug_line;

void bsp_debug_uart_init(void)
{
    g_last_debug_line = 0;
}

void bsp_debug_uart_write_line(const char *line)
{
    g_last_debug_line = line;
}
