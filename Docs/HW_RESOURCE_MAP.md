# 硬件资源映射

证据优先级按当前官方库和资料高于去年代码。表中“输出启用”不是“信号存在”；真实车台架前，危险执行器仍保持禁能。

| 模块 | 信号名 | 实际引脚/资源 | 驱动文件 | 初始化函数 | 依据路径 | 证据等级 | 置信度 | 未验证风险 |
|---|---|---|---|---|---|---|---:|---|
| AI8051U | 系统时钟 | `MAIN_Fosc=40000000L` | `User/config.h` | `System_Init()` | 官方当前库 `4-LQ_AI8051U_LIB/User/config.h` | 官方源码 | 90% | 去年代码为 42 MHz，需确认使用哪套库下载 |
| Keil 工程 | C251 v5.60 | AI8051U-32Bit Series | `MDK_Project/smartcar_fyzw.uvproj` | 官方 target 配置 | 官方 `LQ_AI8051U_32Bit_LIB.uvproj` | 官方工程 | 90% | 已在 `AI8051U_FYZW_SAFE` 真实 Rebuild 并生成 HEX |
| 调试串口 | UART1 | P30/P31, 115200 | `BSP/bsp_debug_uart.c` | `UART_Configuration(UART1,...)` | 综合测试 `User/main.c` + official Driver | 官方例程 | 85% | 串口物理口需下载调试器确认 |
| 有刷推进 | DRV8701 | PWM5/P50、PWM7/P52，方向 P51/P53 | `BSP/bsp_drive.c` | `PWM_Configuration`, `UpdatePwmCh`, `gpio_write_pin` | `LQ_MotorServo.c/.h` | 官方源码 | 80% | 电机极性和实车左右轮方向需离地确认 |
| 转向/舵机 | historical/inactive | PWM1/P10、PWM2/P12 不在主路径使用 | `BSP/bsp_steering.c` | 不由 scheduler 调用 | 旧安全参考 | 历史文件 | 20% | 当前车为无舵机差速车，不得接入主控制链 |
| 风机 ESC | 无刷 PWM | P2.2，PWMA PWM2P_3；50 Hz；1-2 ms 高电平 | `BSP/bsp_fan_esc.c` | `PWM2_USE_P22P23`, `PWM_Configuration`, `UpdatePwmCh` | 用户固定硬件事实 + 官方 PWM API | 软件映射已接入 | 80% | `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0`，真实输出仍禁用；需示波和固定工装验证 |
| ESC 输入 | PWMIN | ESC 板 P00/P01 同一 PWMIN；GND 必须接；50-300 Hz；1-2 ms | ESC 手册/引脚说明 | 外部信号输入 | `LQ-STC_BLDC_MINI_V3...` | 官方资料 | 85% | MCU 到 ESC 接线、供电、方向、故障输出未闭环 |
| 编码器 | Enc1/Enc2 | Enc1: T3 A=P04 DIR=P05；Enc2: T4 A=P06 DIR=P07；原理图信号 `ENC1A/B`,`ENC2A/B` | `BSP/bsp_encoder.c` | T3/T4 外部计数，读后清零 | 官方源码和 AI8051U 原理图 | 官方源码+原理图 | 80% | 轮径、线数、方向需实测 |
| 5LC 电磁 | 5 路 ADC 算法资产 | 去年顺序 ADC5,4,3,0,1 -> L1,L2,M,R1,R2；官方 ADC 初始化含 CH0-5, CH9 | `LQ_ADC.c`, legacy `Inductor.c` | `ADC_Init`, `Get_ADCResult` | 官方 ADC 源码 + 去年代码 | 部分证据 | 60% | 5LC 到 AI8051U 实际通道未从原理图完全闭环 |
| LSM6DSR | IMU SPI/I2C | 原理图文本显示 SPIMISO/SPIMOSI/SPICLK/SPICS 与 SDO/SA0/SDA/SCL/CS；数据手册支持 I2C/SPI | `LQ_LSM6DSR_Hard.c/.h` | `Test_LSM6DSR_Hard`, `LSM6DSR_Init` | 官方源码、原理图、数据手册 | 中等 | 75% | 安装方向和坐标轴必须实测 |
| UI | OLED/KEY/LED | 官方测试提供 GPIO_LED、KEY、OLED；具体按官方库 | `LQ_GPIO_LED.c`, `LQ_KEY.c`, `LQ_OLED096.c` | 官方测试函数 | 综合测试工程 | 官方例程 | 75% | 人工授权键位需实车确认 |
| 电源采样 | 电池电压 | 去年使用 ADC8 经验公式 | legacy `ld.c` | `Read_Battery_Voltage` | 去年代码 | 旧工程经验 | 40% | 当前板实际电压分压未确认，不作硬故障依据 |

## 资源冲突

- PWMA 当前主路径使用 PWM2P/P2.2 作为风机 ESC 资源；舵机 PWM1/PWM2 为历史禁用资源，不得重新接入主路径。
- PWMB 用于推进 PWM5/PWM7；不能再分配给负压。
- 编码器占用 T3/T4 计数输入；不能把 T3/T4 再作为普通时基或串口波特率依赖。
- 官方综合测试是单模块死循环测试，不等于整车调度架构。
