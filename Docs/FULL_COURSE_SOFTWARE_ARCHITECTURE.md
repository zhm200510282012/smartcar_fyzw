# 完整赛道软件架构

当前主架构是五路电磁差速车，不再把模糊 PID 输出接到舵机。主路径固定为：

```text
A-E 五路电磁
-> 线误差、滤波误差、误差变化率、质量、丢线状态
-> 模糊 PID / 基础 PD
-> turn_delta_mm_s
-> 左右目标轮速
-> 左右独立速度 PI
-> 左右电机 PWM/方向
-> P2.2 风机逻辑请求
-> 安全仲裁
```

## 调度顺序

1. 读取电磁、编码器、IMU、UI、路线事件。
2. `ctrl_line_update()` 计算五路加权重心。
3. `track_wall_logic_update()` 处理上墙/墙面/下墙软件状态。
4. `track_strategy_mode_params()` 选取低速保守参数。
5. `ctrl_fuzzy_turn_update()` 输出 `turn_delta_mm_s`。
6. `ctrl_differential_drive_mix()` 得到左右目标轮速。
7. `ctrl_speed_update()` 分别生成左右 native 驱动命令。
8. `ctrl_adhesion_update()` 生成风机逻辑请求。
9. `app_safety_apply_profile()` 与 `app_output_arbitrate()` 做最后安全仲裁。
10. `bsp_drive_apply_lr()` 输出左右电机，`bsp_fan_esc_apply()` 在物理许可时才输出 P2.2 PWM。

## 状态集合

| 状态 | 用途 | 真实硬件放行 |
| --- | --- | --- |
| `BOOT` | 上电初始化 | 是 |
| `SELF_CHECK` | 基础自检 | 低风险 |
| `SENSOR_CALIBRATION` | 传感稳定 | 5LC/IMU 仍需实车校准 |
| `SAFE_GROUND_READY` | 等待人工 arm | 真实默认未 arm |
| `ARMED_GROUND` | 地面 arm 后入口 | 仅允许低风险地面测试 |
| `GROUND_TRACK` | 地面差速循迹 | 需 5LC/编码器确认后测试 |
| `TRANSITION_CANDIDATE` | 上墙候选兼容状态 | 当前不代表真实上墙许可 |
| `SUCTION_PRECHARGE` | 风机预充兼容状态 | 物理 PWM 禁用 |
| `APPROACH_TRANSITION` | 过渡准备兼容状态 | 真实上墙禁用 |
| `TRANSITION_UP` | 上墙过渡 | 真实上墙禁用 |
| `WALL_TRACK` | 墙面循迹 | 真实上墙禁用 |
| `CYLINDER_TRACK` | 曲面/圆柱段 | 真实上墙禁用 |
| `TRANSITION_DOWN` | 下墙过渡 | 真实上墙禁用 |
| `GROUND_RECOVERY` | 落地恢复 | 真实上墙禁用 |
| `FINISHED` | 完赛安全输出 | 驱动 0、风机真实输出 0 |
| `GROUND_FAULT` | 地面故障 | 驱动 0、风机真实输出 0 |
| `SUCTION_LOCKOUT` | 上墙/负压未授权拒绝 | 驱动 0、风机真实输出 0 |
| `WALL_FAILSAFE_HOLD` | 墙面故障逻辑保压 | 逻辑可请求保持，真实输出仍为 0 |
| `HARD_FAULT` | Kill 或非法状态 | 全部输出安全 |

## 安全边界

- `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0`：P2.2/PWM2P_3 不输出真实 ESC 控制脉冲。
- `WALL_RUN_ENABLE=0`：C251 实车不进入真实上墙运行。
- `SUCTION_HW_VERIFIED=0`：旧负压安全门仍保持锁定。
- Host-SIL 只证明软件状态和安全仲裁，不证明风机、附着力、IMU 轴向或真实赛道通过。
