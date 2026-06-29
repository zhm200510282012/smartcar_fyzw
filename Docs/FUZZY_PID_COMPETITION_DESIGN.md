# 模糊 PID 竞赛控制设计

本工程只保留一套定点模糊 PID。当前实时路径为差速车路径：

```text
line_error_filtered, line_error_rate
-> ctrl_fuzzy_pid_eval()
-> Kp/Ki/Kd 自整定
-> ctrl_fuzzy_turn_update()
-> turn_delta_mm_s
-> ctrl_differential_drive_mix()
-> left/right target speed
```

`FUZZY_ENABLE=0` 时，控制器使用基础 PD 参数；`FUZZY_ENABLE=1` 时，5x5 Sugeno 规则只调整同一套控制器的增益，不新增第二套 PID。

## 主路径文件

| 文件 | 当前职责 |
| --- | --- |
| `Control/ctrl_fuzzy_pid.c` | 定点 5x5 模糊推理和增益限幅 |
| `Control/ctrl_fuzzy_turn.c` | 将误差转换为 `turn_delta_mm_s` |
| `Control/ctrl_differential_drive.c` | 将转向差速混控为左右目标速度 |
| `Control/ctrl_speed.c` | 左右独立速度 PI |
| `App/app_scheduler.c` | 串接五路电磁、差速、PI、风机和安全仲裁 |

旧 `ctrl_fuzzy_steering.c`、`ctrl_steering.c`、`ctrl_vehicle.c` 仅为历史文件，不在 Keil 主目标的调度输出路径中使用。

## 安全复位

未 Arm、完成、故障、丢线停车、Kill、`SUCTION_LOCKOUT`、`WALL_FAILSAFE_HOLD` 时，模糊 PID 状态复位，`turn_delta_mm_s=0`，左右目标速度和左右 native 输出为 0。
