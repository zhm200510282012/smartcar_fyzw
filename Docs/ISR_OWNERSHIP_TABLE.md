# ISR 唯一所有权表

原则：每个硬件中断向量只允许一个拥有者。当前目标没有加入任何 `Drivers\Official_LQ\Driver\isr\*.c`；唯一新增 ISR 为本工程 `bsp_timebase.c` 的 Timer0 1 ms 节拍。

| 中断向量 | 当前拥有模块 | 是否启用 | 是否与官方 ISR 冲突 | 最终处理 |
| ---- | ------ | ---: | ------------ | ---- |
| `TMR0_VECTOR` | `BSP/bsp_timebase.c::bsp_timebase_timer0_isr` | 是 | 否 | Timer0 只递增 1 ms tick；不加入 `AI8051U_Timer_Isr.c` |
| `TMR1_VECTOR` | 无 | 否 | 否 | 不加入官方 Timer ISR |
| `TMR2_VECTOR` | UART1 baud generator by `AI8051U_UART.c` | 否 | 否 | 不定义 ISR；仅由 UART1 初始化用作波特率 |
| `TMR3_VECTOR` | 无 | 否 | 否 | T3 用作左编码器外部计数，不加入 Timer3 ISR |
| `TMR4_VECTOR` | 无 | 否 | 否 | T4 用作右编码器外部计数，不加入 Timer4 ISR |
| `TMR11_VECTOR` | 无 | 否 | 否 | 不加入官方 Timer ISR |
| `ADC_VECTOR` | 无 | 否 | 否 | 5LC ADC 映射未验证，不加入 `AI8051U_ADC_Isr.c` |
| `DMA_ADC_VECTOR` | 无 | 否 | 否 | ADC DMA 未启用，不加入 `AI8051U_DMA_Isr.c` |
| `DMA_UR1T_VECTOR` | 无 | 否 | 否 | Debug UART 使用轮询发送；不加入 UART DMA ISR |
| `DMA_UR1R_VECTOR` | 无 | 否 | 否 | Debug UART 不启用接收 DMA |
| `DMA_SPI_VECTOR` | 无 | 否 | 否 | SPI DMA 未启用 |
| `SPI_VECTOR` | 无 | 否 | 否 | IMU bus/orientation 未验证；不为凑文件加入 `AI8051U_SPI_Isr.c` |
| `UART1_VECTOR` | 无 | 否 | 否 | `bsp_debug_uart.c` 直接使用 `AI8051U_UART.c` 轮询发送；不加入 `AI8051U_UART_Isr.c` |
| `UART3_VECTOR` | 无 | 否 | 否 | 不加入官方 UART ISR |
| `UART4_VECTOR` | 无 | 否 | 否 | 不加入官方 UART ISR |

若后续启用官方 UART DMA、硬件 SPI IMU、ADC 或新增定时器中断，必须先在本表声明唯一拥有者，再把对应 ISR 文件纳入 `.uvproj`。
