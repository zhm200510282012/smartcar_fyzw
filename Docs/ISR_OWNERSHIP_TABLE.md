# ISR 唯一所有权表

原则：每个硬件中断向量只允许一个拥有者。当前工程不引入官方 `Driver/isr/*.c` 的整包 ISR，也不使用 Timer11 扩展中断。

| 外设/定时器 | 中断向量 | 当前用途 | 当前拥有模块 | 优先级 | 周期/频率 | 是否读取 ADC | 是否调用 PID | 是否写电机/风机 | 最终处理 |
|---|---|---|---|---:|---:|---:|---:|---:|---|
| Timer0 | `TMR0_VECTOR` | 系统毫秒时基和 5 分频控制 tick | `BSP/bsp_timebase.c::bsp_timebase_timer0_isr` | `BSP_CONTROL_TIMER_PRIORITY=1` | `TIMEBASE_TICK_HZ=1000 Hz` | 否 | 是，每 5 ms 且仅新 frame | 是，仅仲裁后 | 保留本工程 ISR，不加入官方 Timer ISR |
| Timer1 | `TMR1_VECTOR` | 单通道电磁 ADC tick | `BSP/bsp_control_timers.c::bsp_sensor_timer1_isr` | `BSP_SENSOR_TIMER_PRIORITY=3` | `SENSOR_ADC_TICK_HZ=1000 Hz` | 是，每次 1 路 | 否 | 否 | 本工程独占；五个 tick 发布 200 Hz 完整 frame |
| Timer2 | `TMR2_VECTOR` | UART1 baud generator | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 只作 UART1 波特率资源 |
| Timer3 | `TMR3_VECTOR` | 左编码器计数 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 只作外部计数资源 |
| Timer4 | `TMR4_VECTOR` | 右编码器计数 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 只作外部计数资源 |
| ADC | `ADC_VECTOR` | 未启用 ADC 中断 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 五路电磁由 Timer1 分时轮询 |
| DMA/SPI/UART ISR | 多个向量 | 未启用 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 不为凑文件加入官方 ISR |

## 调用边界

- `Timer1` 只推进 `bsp_emag_sensor_tick()`，不调用 line、fuzzy、speed PI、wall logic 或输出仲裁。
- `Timer0` 固定先执行 `g_ms++`，再按 `TIMEBASE_TICK_HZ / CONTROL_PID_HZ` 分频调用 `app_control_tick_control_isr()`。
- Timer1 优先级高于 Timer0；当第五次采样和控制 tick 同一时刻到来时，Timer1 完成 frame 发布后 Timer0 控制 tick 再消费新 sequence。
- Host-SIL 通过 runner 模拟 `Timer1=1 ms`、`Timer0 control=5 ms`，不在同一毫秒伪造多个实时控制周期。
- Timer1/Timer0 ISR 内有 no-op timing scope hook，但默认 `BSP_TIMING_SCOPE_ENABLE=0`，不会占用任何 GPIO。
