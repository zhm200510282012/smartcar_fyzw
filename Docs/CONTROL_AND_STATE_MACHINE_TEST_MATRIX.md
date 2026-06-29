# 控制与状态机测试矩阵

Host-SIL 使用两个 profile：

| Profile | 用途 | 负压真实输出 |
| --- | --- | ---: |
| `guard` | 验证真实 C251 安全门控等价路径，墙状态不可用 | 0 |
| `logical_wall` | 仅在 Host-SIL 中验证完整墙面逻辑状态流 | 0 |

## S01-S20

| 场景 | Profile | 目标 | 结果 |
| --- | --- | --- | --- |
| S01 ground straight | guard | 地面直线，不进入墙相关状态 | PASS |
| S02 ground curve | guard | 地面左右线误差，转向限幅 | PASS |
| S03 precharge before transition | logical_wall | 预充早于上墙姿态观测 | PASS |
| S04 wall track | logical_wall | 上墙到墙面循迹状态链路 | PASS |
| S05 wall to ground transition | logical_wall | 墙面到落地恢复并完赛 | PASS |
| S06 curved/cylinder mode | logical_wall | 圆柱/曲面状态进入与恢复 | PASS |
| S07 stale sensor on ground | guard | 地面传感 stale 进入地面故障 | PASS |
| S08 stale sensor on wall | logical_wall | 墙面 stale 进入 wall failsafe | PASS |
| S09 transition timeout | logical_wall | 上墙超时进入 wall failsafe | PASS |
| S10 suction unverified lockout | guard | 未验证负压时拒绝墙路径 | PASS |
| S11 encoder dropout | logical_wall | 墙面编码器失效进入 failsafe | PASS |
| S12 emag loss | logical_wall | 墙面电磁信号丢失进入 failsafe | PASS |
| S13 IMU abnormal | logical_wall | IMU ID/轴异常进入 failsafe | PASS |
| S14 unarmed steering centered | guard | 未 arm 时驱动 0、舵机回中 | PASS |
| S15 finished steering centered | logical_wall | 完赛后驱动 0、舵机回中 | PASS |
| S16 illegal state hard fault | guard | 非法状态进入 hard fault | PASS |
| S17 transition-down no rebound up | logical_wall | 下墙后不允许回弹到上墙路径 | PASS |
| S18 single-frame IMU spike rejection | guard | 单帧地面 pitch spike 不触发上墙 | PASS |
| S19 kill overrides all outputs | logical_wall | kill 后全部输出归安全态 | PASS |
| S20 control-period overrun fault | guard | 控制周期异常进入地面故障 | PASS |

结果文件：

- `sim/results/full_course_summary.json`
- `sim/results/full_course_summary.csv`
- `build/host_sil_after_full_logic.log`

当前统计：20/20 pass；`logical_wall` 最大逻辑吸附请求为 1200，最大真实硬件负压输出为 0。
