# 上墙/墙面/下墙状态机

状态机入口是标准路线事件 `track_route_event_t.wall_approach_event`。当前没有可靠前置事件时，实车必须保持 `GROUND_TRACK`，不能用 pitch 已经上升来倒推预充压。

| 状态 | 驱动策略 | 风机逻辑 | 进入条件 | 离开条件 |
| --- | --- | --- | --- | --- |
| `GROUND_TRACK` | 正常地面差速循迹 | `OFF` | 默认地面运行 | `wall_approach_event` 且墙逻辑允许 |
| `WALL_APPROACH` | 降速、准备预充 | `OFF` | 上墙前置事件出现 | 下一控制周期进入 `FAN_PRECHARGE`；事件撤销则回地面 |
| `FAN_PRECHARGE` | 很低速或暂停 | `PRECHARGE` | 上墙前置事件保持 | 预充时间达到且 pitch 连续超过 `IMU_WALL_ENTER_CDEG` 后进入 `TRANSITION_UP` |
| `TRANSITION_UP` | 低速差速 | `HOLD` | 预充完成后的上墙姿态确认 | 墙面 pitch 连续确认后进入 `WALL_TRACK`；超时则 `WALL_FAILSAFE_HOLD` |
| `WALL_TRACK` | 墙面低速循迹 | `HOLD` | 墙面姿态连续确认 | 下墙 pitch 低于 `IMU_WALL_EXIT_CDEG` 后进入 `TRANSITION_DOWN`；圆柱/曲面事件进入 `CYLINDER_TRACK` |
| `CYLINDER_TRACK` | 更低速、限制差速 | `HOLD`/`BOOST` | 墙面中出现曲面姿态 | 回到墙面姿态则 `WALL_TRACK`；低于下墙阈值则 `TRANSITION_DOWN` |
| `TRANSITION_DOWN` | 很低速 | `HOLD` | 下墙姿态连续确认 | 地面姿态连续达到 `IMU_GROUND_CONFIRM_MS` 后进入 `GROUND_RECOVERY` |
| `GROUND_RECOVERY` | 低速恢复 | `RAMP_DOWN` | 落地确认 | 继续稳定后回 `GROUND_TRACK` |
| `WALL_FAILSAFE_HOLD` | 左右电机 0 | `FAILSAFE_HOLD` | 墙相关状态 IMU stale、超时、状态矛盾、编码器/电磁危险 | 人工复位或重新上电 |

## 关键规则

- 预充压必须发生在姿态变化之前。
- `TRANSITION_UP` 与 `TRANSITION_DOWN` 使用不同阈值，形成滞回。
- `TRANSITION_DOWN` 不允许反跳回 `TRANSITION_UP`；若 pitch 再次超过上墙阈值，进入 `WALL_FAILSAFE_HOLD`。
- `WALL_FAILSAFE_HOLD` 在逻辑层可请求风机保持，但当前物理 PWM 禁用，真实输出仍为 0。
- `GROUND_RECOVERY` 连续确认后才允许风机渐降。
