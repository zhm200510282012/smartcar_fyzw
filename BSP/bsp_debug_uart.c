#ifndef HOST_SIL
#include "AI8051U_GPIO.h"
#include "AI8051U_Switch.h"
#include "AI8051U_UART.h"
#endif
#include "bsp_debug_uart.h"
#include "board_map.h"

static const char * volatile g_last_debug_line;

void bsp_debug_uart_init(void)
{
    g_last_debug_line = 0;
#ifndef HOST_SIL
    if (BOARD_DEBUG_UART_VERIFIED != 0) {
        COMx_InitDefine uart;
        UART1_SW(0u);
        gpio_init_pin(P3_0, GPIO_Mode_IN_FLOATING);
        gpio_init_pin(P3_1, GPIO_Mode_Out_PP);
        uart.UART_Mode = UART_8bit_BRTx;
        uart.UART_BRT_Use = BRT_Timer2;
        uart.UART_BaudRate = 115200ul;
        uart.Morecommunicate = DISABLE;
        uart.UART_RxEnable = DISABLE;
        uart.BaudRateDouble = DISABLE;
        UART_Configuration(UART1, &uart);
        TI = 0;
    }
#endif
}

void bsp_debug_uart_write_line(const char *line)
{
    g_last_debug_line = line;
#ifndef HOST_SIL
    if (BOARD_DEBUG_UART_VERIFIED != 0 && line != 0) {
        const char *p;
        p = line;
        while (*p != '\0') {
            SBUF = (u8)*p;
            while (TI == 0) {
            }
            TI = 0;
            p++;
        }
        SBUF = (u8)'\r';
        while (TI == 0) {
        }
        TI = 0;
        SBUF = (u8)'\n';
        while (TI == 0) {
        }
        TI = 0;
    }
#endif
}
