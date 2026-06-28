# ISR 唯一所有权表

本轮原则：每个硬件中断向量只允许一个拥有者。当前目标没有加入任何 `Drivers\Official_LQ\Driver\isr\*.c`，因为当前 BSP 尚未启用 Timer/ADC/DMA/SPI/UART 中断服务函数。

| 中断向量 | 当前拥有模块 | 是否启用 | 是否与官方 ISR 冲突 | 最终处理 |
| ---- | ------ | ---: | ------------ | ---- |
| `TMR0_VECTOR` | 无 | 否 | 否 | 不加入 `AI8051U_Timer_Isr.c`；`bsp_timebase.c` 仅提供 `bsp_timebase_on_tick_1ms()`，没有 ISR 定义 |
| `TMR1_VECTOR` | 无 | 否 | 否 | 不加入官方 Timer ISR |
| `TMR3_VECTOR` | 无 | 否 | 否 | 不加入官方 Timer ISR |
| `TMR4_VECTOR` | 无 | 否 | 否 | 不加入官方 Timer ISR |
| `TMR11_VECTOR` | 无 | 否 | 否 | 不加入官方 Timer ISR |
| `ADC_VECTOR` | 无 | 否 | 否 | 不加入 `AI8051U_ADC_Isr.c` |
| `DMA_ADC_VECTOR` | 无 | 否 | 否 | 不加入 `AI8051U_DMA_Isr.c` |
| `DMA_UR1T_VECTOR` | 无 | 否 | 否 | `LQ_UART.c` 已从目标移除，当前不需要 `DmaTx1Flag` |
| `DMA_UR1R_VECTOR` | 无 | 否 | 否 | `LQ_UART.c` 已从目标移除，当前不需要 `DmaRx1Flag` |
| `DMA_SPI_VECTOR` | 无 | 否 | 否 | SPI DMA 未启用，不加入官方 DMA ISR |
| `SPI_VECTOR` | 无 | 否 | 否 | IMU 官方头文件默认 `HARDWARE_SPI`，但当前 `bsp_imu.c` 未调用官方 SPI IMU路径；不为凑文件加入 `AI8051U_SPI_Isr.c` |
| `UART1_VECTOR` | 无 | 否 | 否 | `bsp_debug_uart.c` 不依赖 `LQ_UART.c`；不加入 `AI8051U_UART_Isr.c` |
| `UART3_VECTOR` | 无 | 否 | 否 | 不加入官方 UART ISR |
| `UART4_VECTOR` | 无 | 否 | 否 | 不加入官方 UART ISR |

若后续 BSP 真正启用官方 UART DMA、硬件 SPI IMU、ADC 或 Timer 中断，必须先在本表声明唯一拥有者，再加入对应 ISR 文件。
