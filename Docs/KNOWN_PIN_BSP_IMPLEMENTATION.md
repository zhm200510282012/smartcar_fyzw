# 已知引脚 BSP 实现

## 已接入的已知资源

| 功能 | AI8051U 资源 | BSP 文件 | 实现状态 | 安全边界 |
| --- | --- | --- | --- | --- |
| 左驱动 PWM | PWM5/P50 | `BSP/bsp_drive.c` | 调用 `PWM5_USE_P50()`、`UpdatePwmCh(PWM5, duty)` | 命令限幅 `+/-1000` |
| 右驱动 PWM | PWM7/P52 | `BSP/bsp_drive.c` | 调用 `PWM7_USE_P52()`、`UpdatePwmCh(PWM7, duty)` | 命令限幅 `+/-1000` |
| 左驱动方向 | P51 | `BSP/bsp_drive.c` | `gpio_write_pin(P5_1, dir)` | 方向极性需空载确认 |
| 右驱动方向 | P53 | `BSP/bsp_drive.c` | `gpio_write_pin(P5_3, dir)` | 方向极性需空载确认 |
| 舵机 1 | PWM1/P10 | `BSP/bsp_steering.c` | 50 Hz，`PWM1_USE_P10P11()`，只启用 PWM1P | 中值 1510 us，机械限位未实测 |
| 舵机 2 | PWM2/P12 | `BSP/bsp_steering.c` | 50 Hz，`PWM2_USE_P12P13()`，只启用 PWM2P | 与 P23 负压候选共享 PWM2 通道 |
| 左编码器 | T3/P04 + DIR/P05 | `BSP/bsp_encoder.c` | T3 外部计数，P05 判方向，读后清零 | 线数、轮径、方向需标定 |
| 右编码器 | T4/P06 + DIR/P07 | `BSP/bsp_encoder.c` | T4 外部计数，P07 判方向，读后清零 | 线数、轮径、方向需标定 |
| Debug UART | UART1/P30/P31 | `BSP/bsp_debug_uart.c` | 115200，Timer2 波特率，轮询发送 | 不启用 UART ISR/DMA |
| 1 ms 时基 | Timer0 ISR | `BSP/bsp_timebase.c` | `TMR0_VECTOR` 唯一 owner | ISR 只递增 tick |

## 仍锁定的资源

| 功能 | 当前宏 | 状态 |
| --- | ---: | --- |
| 负压输出 P23/PWM2N | `SUCTION_HW_VERIFIED=0`，`BOARD_SUCTION_SIGNAL_VERIFIED=0` | 真实输出保持 0 |
| 5LC ADC 映射 | `BOARD_EMAG_ADC_VERIFIED=0` | 不作为真实闭环依据 |
| IMU bus/orientation | `BOARD_IMU_BUS_VERIFIED=0` | 不允许上墙 |
| 电源采样 | `BOARD_POWER_SENSE_VERIFIED=0` | 真实自检默认不放行 |
| UI/OLED/按键 | `BOARD_UI_VERIFIED=0` | 真实默认不 arm |

## 负压与 PWM2 冲突记录

P23 负压候选为 PWM2N，但当前第二舵机使用 PWM2P/P12，二者共享 PWM2 通道周期和 duty。由于负压链路未验证，当前 BSP 不配置 P23，不输出非零值，不把该路径作为可用硬件。
