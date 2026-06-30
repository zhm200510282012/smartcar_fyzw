# ISR 唯一所有权表

原则：每个硬件中断向量只允许一个拥有者。当前工程不引入官方 `Driver/isr/*.c` 的整包 ISR。

| 外设/定时器 | 中断向量 | 当前用途 | 当前拥有模块 | 优先级 | 周期/频率 | 是否读取 ADC | 是否调用 PID | 是否写电机/风机 | 最终处理 |
|---|---|---|---|---:|---:|---:|---:|---:|---|
| Timer0 | `TMR0_VECTOR` | 1 ms 系统时基 | `BSP/bsp_timebase.c::bsp_timebase_timer0_isr` | 官方默认 | 1 ms | 否 | 否 | 否 | 保留本工程 ISR，不加入官方 Timer ISR |
| Timer1 | `TMR1_VECTOR` | 单通道电磁 ADC tick | `BSP/bsp_control_timers.c::bsp_sensor_timer1_isr` | `BSP_SENSOR_TIMER_PRIORITY=3` | `SENSOR_ADC_TICK_HZ=1000 Hz` | 是，每次 1 路 | 否 | 否 | 由本工程独占；五个 tick 发布 200 Hz 完整 frame |
| Timer11 | `TMR11_VECTOR` | 完整控制链 tick | `BSP/bsp_control_timers.c::bsp_control_timer11_isr` | `BSP_CONTROL_TIMER_PRIORITY=2` | `CONTROL_PID_HZ=200 Hz` | 否 | 是，仅新 frame | 是，仅仲裁后 | 由本工程独占；不得快于 `SENSOR_FRAME_HZ` |
| Timer2 | `TMR2_VECTOR` | UART1 baud generator | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 只作 UART1 波特率资源 |
| Timer3 | `TMR3_VECTOR` | 左编码器计数 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 只作外部计数资源 |
| Timer4 | `TMR4_VECTOR` | 右编码器计数 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 只作外部计数资源 |
| ADC | `ADC_VECTOR` | 未启用 ADC 中断 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 五路电磁由 Timer1 分时轮询 |
| DMA/SPI/UART ISR | 多个向量 | 未启用 | 无 ISR | 不适用 | 不适用 | 否 | 否 | 否 | 不为凑文件加入官方 ISR |

## 调用边界

- `Timer1` 只推进 `bsp_emag_sensor_tick()`，不调用 line、fuzzy、speed PI、wall logic 或输出仲裁。
- `Timer11` 只消费新完整 frame。无新 frame 时保持上一输出；stale frame 时进入安全路径。
- Host-SIL 通过 scheduler 模拟 `Timer1=1 ms`、`Timer11=5 ms`，不在同一毫秒伪造多个实时控制周期。
- Timer1/Timer11 ISR 内有 no-op timing scope hook，但默认 `BSP_TIMING_SCOPE_ENABLE=0`，不会占用任何 GPIO。
