# 软件链路调试顺序

这份清单只用于把软件控制链跑通和准备台架，不是上墙许可。

## 先只看 Host-SIL

1. 运行差速/上墙逻辑 SIL，确认 D01-D09、W01-W11 全部通过。
2. 运行旧 P0、full-course、fuzzy 回归，确认安全门没有被破坏。
3. 检查 `differential_wall_logic_summary.json` 中 `hardware_fan_output_max=0`。

## 离地差速调试

1. 保持 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0`、`WALL_RUN_ENABLE=0`。
2. 离地确认电机 1 是左轮、电机 2 是右轮。
3. 确认左右编码器正方向和速度新鲜度。
4. 手持车扫过电磁线，确认 A-E 左到右顺序和 `line_error` 符号。
5. `FUZZY_ENABLE=0`，先让基础差速 PD 低速不摆振。
6. 基础 PD 正常后再把 `FUZZY_ENABLE` 改为 1 做自整定测试。

## 风机台架前置

仅当示波确认 P2.2/PWM2P_3 到 ESC PWMIN、GND 共地、波形频率和脉宽正确后，才允许进入固定工装风机台架。开始台架仍保持 `WALL_RUN_ENABLE=0`。

台架只需要集中修改 `App/competition_profile.h`：

```c
FAN_ESC_PHYSICAL_OUTPUT_ENABLE
FAN_PWM_FREQ_HZ
FAN_ESC_MIN_US
FAN_ESC_MAX_US
FAN_PRECHARGE_US
FAN_HOLD_US
FAN_BOOST_US
```

不需要改 scheduler、状态机、BSP 或 PID 表。
