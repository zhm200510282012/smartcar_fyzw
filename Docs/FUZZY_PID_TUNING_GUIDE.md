# 模糊 PID 差速调试指南

当前模糊 PID 不再输出舵机脉宽。它只根据五路电磁误差输出 `turn_delta_mm_s`，再由差速混控生成左右目标轮速。

## 调试顺序

1. 保持 `FUZZY_ENABLE=0`，先用基础 PD 跑低速地面循迹。
2. 确认 `LINE_DIRECTION_SIGN` 与 `DIFF_TURN_SIGN` 后再提高速度。
3. 基础 PD 不摆振后，将 `FUZZY_ENABLE` 改为 1。
4. 观察 `line_error_filtered`、`line_error_rate`、`turn_delta_mm_s`、左右目标轮速和左右 native 输出。
5. 丢线、未 Arm、完成、故障、Kill 时，`turn_delta_mm_s` 和左右输出必须归零。

## 关键参数

| 参数 | 用途 |
| --- | --- |
| `GROUND_STEERING_KP` | 基础差速 P |
| `GROUND_STEERING_KD` | 基础差速 D |
| `FUZZY_ENABLE` | 是否启用 5x5 Sugeno 自整定 |
| `DIFF_TURN_DELTA_LIMIT_MM_S` | 差速转向速度差限幅 |
| `DIFF_TURN_SIGN` | 差速左右方向 |

Host-SIL 通过不代表实车通过；风机和上墙仍由 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0`、`WALL_RUN_ENABLE=0` 锁定。
