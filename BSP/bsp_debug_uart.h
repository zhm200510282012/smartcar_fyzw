#ifndef BSP_DEBUG_UART_H
#define BSP_DEBUG_UART_H

#include "../App/app_types.h"

void bsp_debug_uart_init(void);
void bsp_debug_uart_write_line(const char *line);

#endif
