# 完整赛道软件架构

## 调度闭环

`App/main.c` 初始化 BSP 和应用状态机后，循环调用 `app_scheduler_run_due()`。真实 C251 目标由 `BSP/bsp_timebase.c` 的 Timer0 ISR 提供 1 ms tick；Host-SIL 使用手动 tick，不访问寄存器。

控制链路顺序固定为：

1. BSP 读取 5LC/IMU/编码器/电源/UI 输入。
2. `ctrl_line_update()` 计算 5 通道线误差和信号质量。
3. `track_surface_state_update()` 和 `track_state_machine_step()` 生成地面、过渡、墙面、圆柱等表面状态。
4. `app_state_machine_step()` 处理完整赛道状态。
5. 速度、转向、吸附控制器生成软件命令。
6. `app_safety_apply_profile()` 应用故障策略。
7. `app_output_arbitrate()` 作为最后一道输出仲裁。
8. BSP apply 把最终命令写入已知引脚驱动；负压真实输出仍强制 0。

## 状态集合

当前状态机包含：

| 状态 | 用途 | 当前真实硬件放行 |
| --- | --- | --- |
| `BOOT` | 上电初始 | 是 |
| `SELF_CHECK` | 电源/基础自检 | 电源采样仍未验证 |
| `SENSOR_CALIBRATION` | 传感稳定与地面确认 | 5LC/IMU 仍未实车标定 |
| `SAFE_GROUND_READY` | 等待人工 arm | UI 未验证，真实默认不 arm |
| `ARMED_GROUND` | 地面 arm 后入口 | 只允许地面低风险路径 |
| `GROUND_TRACK` | 地面循迹 | 需要 5LC/编码器实测后才允许动测 |
| `TRANSITION_CANDIDATE` | 上墙候选确认 | C251 目标因 `SUCTION_HW_VERIFIED=0` 不进入墙能力路径 |
| `SUCTION_PRECHARGE` | Host-only 墙面逻辑预充 | 真实负压禁能 |
| `APPROACH_TRANSITION` | 进入过渡段 | 真实上墙禁用 |
| `TRANSITION_UP` | 上墙过渡 | 真实上墙禁用 |
| `WALL_TRACK` | 墙面循迹 | 真实上墙禁用 |
| `CYLINDER_TRACK` | 曲面/圆柱段 | 真实上墙禁用 |
| `TRANSITION_DOWN` | 下墙过渡 | 真实上墙禁用 |
| `GROUND_RECOVERY` | 落地恢复 | 真实上墙禁用 |
| `SEESAW_PASS` | 跷跷板占位状态 | 当前仅状态占位，未接实物策略 |
| `FINISHED` | 完赛安全输出 | 驱动归零、舵机回中、负压关断 |
| `GROUND_FAULT` | 地面故障 | 驱动归零、舵机回中、负压关断 |
| `SUCTION_LOCKOUT` | 负压未验证锁定 | 驱动归零、舵机回中、负压关断 |
| `WALL_FAILSAFE_HOLD` | Host-only 墙面故障保持请求 | 真实负压输出仍为 0 |
| `HARD_FAULT` | 硬故障/kill/非法状态 | 全部输出安全态 |

## 安全边界

- C251 目标中 `APP_WALL_STATE_CAPABLE == SUCTION_HW_VERIFIED`，当前为 0。
- Host-SIL `logical_wall` 只验证软件路径和逻辑吸附请求，不证明负压、附着力或上墙能力。
- 真实 BSP 已接入驱动、舵机、编码器、UART、Timer0，但电源、UI、5LC、IMU 仍是未验证路径。
- 负压相关宏 `SUCTION_HW_VERIFIED`、`BOARD_SUCTION_SIGNAL_VERIFIED`、`SUCTION_BENCH_TEST_ENABLE` 均保持 0。
