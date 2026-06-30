# P2.2 风机 ESC 独立台架模式

风机 P2.2/PWM2P_3 映射保持当前状态，默认不输出真实 PWM：

```c
#define FAN_ESC_PHYSICAL_OUTPUT_ENABLE 0
#define WALL_RUN_ENABLE 0
#define SUCTION_HW_VERIFIED 0
#define FAN_BENCH_TEST_ENABLE 0
```

## 正常调用链

正常比赛控制链只有一条：

```text
track_wall_logic_update()
-> ctrl_adhesion_update()
-> app_output_arbitrate()
-> bsp_fan_esc_apply()
-> P2.2/PWM2P_3
```

当 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0` 或 `BOARD_FAN_ESC_BENCH_VERIFIED=0` 时，逻辑请求可以变化，但 `bsp_fan_esc_last_output_us()` 和真实 P2.2 输出必须保持 0。

## 台架模式

`FAN_BENCH_TEST_ENABLE` 是独立台架开关，只允许在固定车体、风机离墙、示波器/安全供电准备完成后临时打开。打开它必须同时满足编译期约束：

```text
FAN_ESC_PHYSICAL_OUTPUT_ENABLE=1
WALL_RUN_ENABLE=0
```

也就是说，台架模式只测 P2.2 到 ESC 的脉宽响应，不允许进入上墙状态机，不允许把 Host-SIL 结果当作实车风机验证。

## 禁止事项

- 不得为了上墙把 `FAN_BENCH_TEST_ENABLE` 长期开启。
- 不得在 `SUCTION_HW_VERIFIED=0` 时宣称负压闭环已验证。
- 不得绕过 `ctrl_adhesion_update()` 直接从状态机写 P2.2。
- 不得改动 P2.2 映射来迁就临时接线。
