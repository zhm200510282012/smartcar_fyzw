# 参数参考

用户后续优先只改 `App/competition_profile.h`。除非新增硬件证据，不要改 scheduler、状态机或 BSP。

## 差速与五路电磁

| 参数 | 默认 | 说明 |
| --- | ---: | --- |
| `LINE_DIRECTION_SIGN` | 1 | 实车确认左右符号后只改这里 |
| `LINE_VALID_SUM_MIN` | 160 | 最小有效总能量 |
| `LINE_LOST_QUALITY_MIN` | 200 | 丢线质量阈值 |
| `LINE_FILTER_ALPHA` | 3 | 误差低通系数，分母 `LINE_FILTER_DENOM=4` |
| `DIFF_TURN_SIGN` | 1 | 差速转向方向，只能 +1/-1 |
| `DIFF_TURN_DELTA_LIMIT_MM_S` | 90 | 差速转向速度差限幅 |
| `DIFF_TARGET_SPEED_LIMIT_MM_S` | 260 | 单轮目标速度限幅 |
| `DIFF_LINE_LOST_SEARCH_SPEED_MM_S` | 45 | 丢线搜索低速 |
| `DIFF_LINE_LOST_STOP_TIME_MS` | 250 | 丢线超时停车 |

## 速度 PI

| 参数 | 默认 | 说明 |
| --- | ---: | --- |
| `SPEED_KP` | 32 | 左右轮共用初始 Kp，PI 状态独立 |
| `SPEED_KI` | 4 | 左右轮共用初始 Ki，PI 状态独立 |
| `SPEED_INTEGRAL_LIMIT` | 3000 | 积分限幅 |
| `SPEED_OUTPUT_LIMIT` | 260 | native 输出限幅 |
| `SPEED_ACCEL_LIMIT` | 12 | 每控制周期加速度/输出变化限制 |

## 模糊与基础 PD

| 参数 | 默认 | 说明 |
| --- | ---: | --- |
| `GROUND_STEERING_KP` | 115 | 基础地面差速 P |
| `GROUND_STEERING_KD` | 70 | 基础地面差速 D |
| `FUZZY_ENABLE` | 0 | 默认先跑基础 PD，稳定后再置 1 |

## 速度档位

| 参数 | 默认 | 说明 |
| --- | ---: | --- |
| `GROUND_STRAIGHT_SPEED_MM_S` | 180 | 保守地面直线速度 |
| `GROUND_CURVE_SPEED_MM_S` | 140 | 保守普通弯速度 |
| `SHARP_CURVE_SPEED_MM_S` | 95 | 急弯/圆柱保守速度 |
| `TRANSITION_SPEED_MM_S` | 70 | 软件过渡速度，不是上墙许可 |
| `WALL_SPEED_MM_S` | 80 | Host-SIL 墙面速度，不是上墙许可 |

## P2.2 风机 ESC

| 参数 | 默认 | 说明 |
| --- | ---: | --- |
| `BOARD_FAN_PWM_MAPPED` | 1 | P2.2/PWM2P_3 资源已固定 |
| `FAN_ESC_PHYSICAL_OUTPUT_ENABLE` | 0 | 真实 P2.2 输出禁用 |
| `WALL_RUN_ENABLE` | 0 | 真实上墙运行禁用 |
| `FAN_PWM_FREQ_HZ` | 50 | RC ESC 默认频率 |
| `FAN_ESC_MIN_US` | 1000 | 最小/arming 脉宽 |
| `FAN_ESC_MAX_US` | 2000 | 最大限幅 |
| `FAN_ESC_ARM_TIME_MS` | 2500 | 台架允许后 arming 时间 |
| `FAN_PRECHARGE_US` | 1350 | 软件预充起点 |
| `FAN_PRECHARGE_TIME_MS` | 500 | 预充持续时间 |
| `FAN_HOLD_US` | 1450 | 软件保持起点 |
| `FAN_BOOST_US` | 1600 | 软件增强起点 |

## IMU 状态机阈值

| 参数 | 默认 | 说明 |
| --- | ---: | --- |
| `IMU_PITCH_SIGN` | 1 | IMU pitch 极性 |
| `IMU_PITCH_OFFSET_CDEG` | 0 | pitch 零偏 |
| `IMU_WALL_ENTER_CDEG` | 4500 | 上墙确认阈值 |
| `IMU_WALL_EXIT_CDEG` | 2500 | 下墙/离墙阈值 |
| `IMU_TRANSITION_CONFIRM_MS` | 150 | 过渡连续确认时间 |
| `IMU_GROUND_CONFIRM_MS` | 250 | 落地连续确认时间 |
| `IMU_STALE_TIMEOUT_MS` | 100 | IMU stale 超时 |

`SUCTION_HW_VERIFIED=0` 作为旧负压安全门继续保留，不代表当前 P2.2 风机已经允许真实输出。
