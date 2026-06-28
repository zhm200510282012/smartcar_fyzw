/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 【平    台】北京龙邱智能科技STC32位核心板
 【编    写】龙邱科技
 【E-mail  】chiusir@163.com
 【软件版本】V1.1 版权所有，单位使用请先联系授权
 【相关信息参考下列地址】
 【网    站】http://www.lqist.cn
 【淘宝店铺】http://longqiu.taobao.com
 --------------------------------------------------------------------------------
 【  IDE  】 keil C251 V5.60
 【Target 】 STC32G/STC8051U/AI8051U 32位模式
 【SYS CLK】 40 MHz使用内部晶振
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/

// 下载时, 选择时钟频率 与"config.h"中宏定义保持一致 默认使用40M主频

#include "include.h"

void main(void)
{
    System_Init();              /* 系统初始化 必须保留 */
    Global_IRQ_Enable();        // 使能全局中断

    // 初始化串口1，使用P30/P31管脚，波特率115200 8位数据,切换定时器需进入该函数内部，默认第一管脚
    UART_Init(UART1_P30_P31, 115200ul);
    GPIO_LED_Init();            // LED管脚初始化,管脚选择在 LQ_GPIO_LED.h 中宏定义
    printf("USER Init OK  \n"); // printf选择 UART1~UART4（在STC32G_UART.h 中宏定义）

    while (1)
    {
        LED_Ctrl(LED_ALL, RVS);
        delay_ms(100);          // 100ms
    }
}
