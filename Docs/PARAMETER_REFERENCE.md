# 参数参考

## 调度周期

| 参数 | 值 | 说明 |
| --- | ---: | --- |
| `TASK_FAST_SENSOR_PERIOD_MS` | 1 | 快速传感采样周期 |
| `TASK_CONTROL_PERIOD_MS` | 2 | 控制周期 |
| `TASK_TRACK_PERIOD_MS` | 20 | 赛道策略周期 |
| `TASK_HEALTH_PERIOD_MS` | 50 | 健康检查周期 |
| `TASK_UI_TELEMETRY_PERIOD_MS` | 100 | UI/遥测周期 |
| `CONTROL_OVERRUN_LIMIT_MS` | 6 | Host-SIL 中的控制周期超时阈值语义 |

## 执行器

| 参数 | 值 | 说明 |
| --- | ---: | --- |
| `DRIVE_LIMIT_ABS` | 1000 | 左右驱动 native 命令限幅 |
| `DRIVE_PWM_FREQ_HZ` | 12500 | 推进 PWM 频率 |
| `STEERING_CENTER_US` | 1510 | 舵机中值 |
| `STEERING_MIN_PULSE_US` | 1000 | 舵机最小脉宽 |
| `STEERING_MAX_PULSE_US` | 2000 | 舵机最大脉宽 |
| `STEERING_MAX_OFFSET_US` | 500 | 线误差转向偏置限幅 |
| `STEERING_PWM_FREQ_HZ` | 50 | 舵机 PWM 频率 |

## 负压

| 参数 | 值 | 说明 |
| --- | ---: | --- |
| `SUCTION_HW_VERIFIED` | 0 | 不允许真实负压输出 |
| `BOARD_SUCTION_SIGNAL_VERIFIED` | 0 | P23 到 ESC 信号链未验证 |
| `SUCTION_BENCH_TEST_ENABLE` | 0 | 负压台架测试未授权 |
| `SUCTION_SAFE_OFF_NATIVE` | 0 | 安全关断 native 值 |
| `SUCTION_PRECHARGE_NATIVE` | 0 | 真实预充输出保持 0 |
| `SUCTION_HOLD_NATIVE` | 0 | 真实保持输出保持 0 |
| `SUCTION_BOOST_NATIVE` | 0 | 真实 boost 输出保持 0 |
| `SUCTION_EMERGENCY_HOLD_NATIVE` | 0 | 真实 emergency hold 输出保持 0 |

Host-SIL `logical_wall` profile 使用逻辑吸附请求覆盖完整状态机测试，但 `bsp_suction_last_native_output()` 仍为 0。

## 状态阈值

| 参数 | 值 | 说明 |
| --- | ---: | --- |
| `LINE_QUALITY_MIN` | 200 | 线信号质量下限 |
| `SENSOR_STALE_TIMEOUT_MS` | 50 | 姿态新鲜度超时 |
| `GROUND_CONFIRM_TIME_MS` | 300 | 落地确认时间 |
| `TRANSITION_TIMEOUT_MS` | 3000 | 过渡超时 |
| `PRECHARGE_MIN_TIME_MS` | 100 | 逻辑预充最小时间 |
| `GROUND_PITCH_MAX_CDEG` | 1200 | 地面 pitch 范围 |
| `TRANSITION_PITCH_CDEG` | 2000 | 过渡 pitch 阈值 |
| `WALL_PITCH_CDEG` | 6000 | 墙面 pitch 阈值 |
| `ADHESION_RISK_LIMIT` | 700 | 吸附风险限速阈值 |
