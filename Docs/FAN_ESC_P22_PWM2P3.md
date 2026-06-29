# P2.2 风机 ESC BSP

无刷负压风机控制信号固定为：

```text
AI8051U P2.2
PWM2P_3
主板“无刷PWM”接口 -> 无刷电调 PWMIN
主板 GND -> 无刷电调 GND
```

不得再使用 P2.3、P1.0/P1.2、PWM1/PWM2 舵机输出作为当前风机或主控制路径。

## 实际初始化方式

`BSP/bsp_fan_esc.c` 使用当前仓库官方 AI8051U 驱动 API：

```c
PWMA_Prescaler((FYZW_MAIN_FOSC_HZ / 1000000ul) - 1ul);
PWM2_USE_P22P23();
GPIO_Init(GPIO_P2, GPIO_Pin_2, GPIO_Mode_Out_PP);
PWM_Configuration(PWM2, &pwm);   // ENO2P, duty 0
PWM_Configuration(PWMA, &pwm);   // period = 1000000 / FAN_PWM_FREQ_HZ
UpdatePwmCh(PWM2, pulse_us);
PWM2P_OUT_EN();
PWM2P_OUT_DIS();
```

默认 `FAN_PWM_FREQ_HZ=50`，周期为 20000 us，脉宽限幅为 `FAN_ESC_MIN_US..FAN_ESC_MAX_US`。

## 物理输出锁定

当前默认：

```c
#define BOARD_FAN_PWM_MAPPED 1
#define FAN_ESC_PHYSICAL_OUTPUT_ENABLE 0
#define WALL_RUN_ENABLE 0
```

当 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0` 时：

```text
fan_request_us 可以在逻辑层变化；
Host-SIL 可以验证状态机；
telemetry 可以记录请求；
真实 P2.2 PWM2P 输出关闭，硬件输出最大值必须为 0。
```

`FAN_PRECHARGE_US`、`FAN_HOLD_US`、`FAN_BOOST_US` 只是软件默认起点，不代表已实车标定。

## 台架前禁止事项

未完成示波和固定工装验证前，不得把 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE` 改为 1，不得把 `WALL_RUN_ENABLE` 改为 1，不得宣称 Host-SIL 通过等于可上墙。
