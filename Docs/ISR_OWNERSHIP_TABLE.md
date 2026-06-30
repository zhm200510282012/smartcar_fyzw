# ISR 唯一所有权表

原则：每个硬件中断向量只允许一个拥有者。当前工程不引入官方 `Driver/isr/*.c` 的整包 ISR；只保留本工程明确使用的中断入口。

| 外设/定时器 | 中断向量 | 当前用途 | 当前拥有模块 | 中断优先级 | 周期 | 是否读取 ADC | 是否调用 PID | 是否写电机/风机输出 | 是否与官方 ISR 冲突 | 最终处理 |
|---|---|---|---|---:|---:|---:|---:|---:|---|---|
| Timer0 | `TMR0_VECTOR` | 1 ms 系统时基 | `BSP/bsp_timebase.c::bsp_timebase_timer0_isr` | 官方默认 | 1 ms | 否 | 否 | 否 | 否 | 保留本工程 ISR，不加入 `AI8051U_Timer_Isr.c` |
| Timer1 | `TMR1_VECTOR` | 五路电磁分时采样 tick | `BSP/bsp_control_timers.c::bsp_sensor_timer1_isr` | `BSP_SENSOR_TIMER_PRIORITY=3` | `SENSOR_FRAME_HZ=1000 Hz` | 是，仅推进一帧采样 | 否 | 否 | 否 | 由本工程独占；不得再加入官方 Timer1 ISR |
| Timer11 | `TMR11_VECTOR` | 控制 PID tick | `BSP/bsp_control_timers.c::bsp_control_timer11_isr` | `BSP_CONTROL_TIMER_PRIORITY=2` | `CONTROL_PID_HZ=500 Hz` | 否 | 是，统一控制链 | 是，最终仲裁后写电机/风机 BSP | 否 | 由本工程独占；不得再加入官方 Timer11 ISR |
| Timer2 | `TMR2_VECTOR` | UART1 baud generator | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | 只作 UART1 波特率资源，不定义 ISR |
| Timer3 | `TMR3_VECTOR` | 左编码器计数 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | 只作外部计数资源，不定义 Timer3 ISR |
| Timer4 | `TMR4_VECTOR` | 右编码器计数 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | 只作外部计数资源，不定义 Timer4 ISR |
| ADC | `ADC_VECTOR` | 未启用 ADC 中断 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | 五路电磁由 Timer1 分时轮询，不加入 `AI8051U_ADC_Isr.c` |
| DMA ADC | `DMA_ADC_VECTOR` | 未启用 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | 不加入官方 DMA ISR |
| SPI | `SPI_VECTOR` | 未启用 SPI 中断 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | IMU 当前不靠 SPI ISR；不为凑文件加入官方 SPI ISR |
| UART1 | `UART1_VECTOR` | Debug UART 轮询发送 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | 不加入官方 UART ISR |
| UART3/UART4 | `UART3_VECTOR`/`UART4_VECTOR` | 未使用 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 否 | 不加入官方 UART ISR |

## 调用边界

- `Timer1` 只调用 `app_control_tick_sensor_isr()`，真实 C251 下推进 `bsp_emag_sensor_tick()`；不得在该 ISR 中调用速度 PI、模糊 PID、状态机或输出仲裁。
- `Timer11` 调用 `app_control_tick_control_isr()`，只消费已完成的电磁帧；不得在该 ISR 中直接做五路 ADC 轮询。
- Host-SIL 为保持可重复测试，仍由 `app_scheduler_run_due()` 触发 sensor/control tick；非 `HOST_SIL` 的真实目标由硬件定时器中断触发快速控制链。
