# P0 安全门设计

P0 安全门的目标是保证未验证的风机和上墙路径不会在 C251 实车中产生危险输出。

## 编译期分离

`App/app_build_profile.h` 将 Host-SIL 逻辑覆盖与真实硬件能力分离：

| 构建 | `APP_WALL_STATE_CAPABLE` | 风机物理输出 |
| --- | ---: | --- |
| C251 `AI8051U_FYZW_SAFE` | `WALL_RUN_ENABLE`，当前 0 | `FAN_ESC_PHYSICAL_OUTPUT_ENABLE`，当前 0 |
| Host-SIL Guard Profile | 0 | 0 |
| Host-SIL Logical Wall Profile | 1 | 0 |

非 Host-SIL 构建中，如果 `WALL_RUN_ENABLE=1` 但 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0`，编译期直接报错。

## Lockout

当上墙请求存在但墙运行未授权时，系统进入 `APP_STATE_SUCTION_LOCKOUT`。该状态是安全拒绝，不是墙面故障保持。

最终仲裁强制：

```text
left_target_speed_mm_s = 0
right_target_speed_mm_s = 0
left_drive_command_native = 0
right_drive_command_native = 0
fan_cmd.state = FAN_ESC_OFF
fan_cmd.output_us = 0
```

## 墙面故障保持

`WALL_FAILSAFE_HOLD` 只在 Host-SIL 或未来授权墙面路径中表达逻辑风机保持请求。当前 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0`，所以真实 P2.2 输出仍为 0。

## 必须继续成立

- 未 Arm、完成、地面故障、硬故障、Kill：左右电机为 0，风机真实输出为 0。
- 线丢失后先低速搜索，超时后停车。
- Host-SIL 可注入 `wall_approach_event`，但 C251 不能伪造上墙前置事件。
- Keil 通过和 Host-SIL 通过都不是实车上墙许可。
