# 完整赛道调车手册

本手册只面向当前五路电磁差速车。没有完成 P0/P1 检查前，不允许上墙。

## 步骤 1：离地确认左右电机正方向

- 检查什么：电机 1 是否为左轮，电机 2 是否为右轮；正命令时车轮是否让车向前。
- 成功标准：左右轮正命令均向前，反命令均向后。
- 失败现象：一侧反转、两侧互换、给小命令时车体想原地转。
- 只改哪个宏：先改 BSP 电机方向映射；不要改 PID 或路线逻辑。
- 禁止做什么：禁止落地大油门试方向。

## 步骤 2：确认左右编码器正方向

- 检查什么：手转左轮只改变左编码器，手转右轮只改变右编码器；车向前推时速度为正。
- 成功标准：左右计数方向一致，速度值随推车速度平滑变化。
- 失败现象：左右互换、向前为负、静止抖动很大。
- 只改哪个宏：编码器方向、轮径、计数比例相关宏。
- 禁止做什么：禁止用速度 PI 去抵消编码器接反。

## 步骤 3：确认 A-E 五路电磁顺序和左右符号

- 检查什么：传感器从车头左到右必须是 A/B/C/D/E。
- 成功标准：线在左侧时左侧通道能量高，线在右侧时右侧通道能量高；`line_error` 左右符号与预期一致。
- 失败现象：扫线时通道顺序跳变，左偏和右偏符号反了。
- 只改哪个宏：`BSP/board_emag_map.h` 和 `LINE_DIRECTION_SIGN`。
- 禁止做什么：禁止在 `ctrl_line.c` 里临时交换数组。

## 步骤 4：低速地面直线，`FUZZY_ENABLE=0`

- 检查什么：基础 PD 是否能让车低速沿直线走。
- 成功标准：不明显蛇形，不丢线，不冲出线。
- 失败现象：向线外打、直线左右摆、丢线后继续冲。
- 只改哪个宏：`GROUND_STEERING_KP`、`GROUND_STEERING_KD`、`LINE_LOST_QUALITY_MIN`。
- 禁止做什么：禁止未跑稳基础 PD 就打开模糊自整定。

## 步骤 5：普通弯、急弯、Omega、环岛调速

- 检查什么：普通弯不外甩，急弯不过冲，Omega/环岛不误判成直线。
- 成功标准：`track_mode` 能进入普通弯或急弯，左右目标速度产生合理差速。
- 失败现象：入弯慢、出弯甩、急弯冲线。
- 只改哪个宏：`GROUND_CURVE_SPEED_MM_S`、`SHARP_CURVE_SPEED_MM_S`、`DIFF_TURN_DELTA_LIMIT_MM_S`。
- 禁止做什么：禁止把十字、环岛方向写死成固定左转或右转。

## 步骤 6：开启 `FUZZY_ENABLE`

- 检查什么：基础 PD 正常后，再让模糊 PID 修正增益。
- 成功标准：同样低速下转向更稳，不产生高频抖动。
- 失败现象：开模糊后明显摆振，或入弯更冲。
- 只改哪个宏：先只改 `FUZZY_ENABLE`；必要时小幅调整 `FUZZY_E_SCALE`、`FUZZY_DE_SCALE`。
- 禁止做什么：禁止同时大改速度、差速限幅和模糊参数。

## 步骤 7：独立风机 ESC 台架

- 检查什么：P2.2 到 ESC PWMIN，GND 共地，50 Hz 脉宽范围。
- 成功标准：离墙、固定车体、低脉宽下 ESC 能按预期响应，并可立即断电。
- 失败现象：ESC 不识别、上电自转、脉宽变化无响应。
- 只改哪个宏：`FAN_ESC_MIN_US`、`FAN_PRECHARGE_US`、`FAN_HOLD_US`、`FAN_BOOST_US`。
- 禁止做什么：禁止把 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE` 改成 1 后直接上墙。

## 步骤 8：IMU 轴向和墙面阈值

- 检查什么：地面 pitch 接近 0，上墙方向 pitch 是否越过 `IMU_WALL_ENTER_CDEG`。
- 成功标准：上墙、墙面、下墙对应 pitch 区间清楚，不与 roll/yaw 混淆。
- 失败现象：上墙时 pitch 反号，静止时漂移，过渡阶段不确认。
- 只改哪个宏：`IMU_PITCH_SIGN`、`IMU_PITCH_OFFSET_CDEG`、`IMU_WALL_ENTER_CDEG`、`IMU_WALL_EXIT_CDEG`。
- 禁止做什么：禁止用 route_event 强行绕过 IMU 方向错误。

## 步骤 9：填写路线事件距离

- 检查什么：上墙前距离、下墙前距离、终点距离。
- 成功标准：只在需要的位置产生 `WALL_APPROACH` 和 `FINISH`。
- 失败现象：提前预充、错过上墙入口、回地面后不结束。
- 只改哪个宏：`ROUTE_PROGRESS_SCRIPT_ENABLE`、`ROUTE_WALL_APPROACH_DISTANCE_MM`、`ROUTE_WALL_EXIT_DISTANCE_MM`、`ROUTE_FINISH_DISTANCE_MM`。
- 禁止做什么：禁止在状态机中写死未知赛道顺序。

## 步骤 10：低速完整赛道

- 检查什么：地面、上墙、墙面、下墙、回地面、终点流程。
- 成功标准：`GROUND_RECOVERY` 后无 `FINISH` 回到 `GROUND_TRACK`；有 `FINISH` 才进入 `FINISHED`。
- 失败现象：下墙后直接结束、未上墙却预充、墙面传感器 stale 后仍有电机输出。
- 只改哪个宏：路线距离、速度和风机脉宽相关宏。
- 禁止做什么：禁止把 Host-SIL 成功当作实车已经通过。

## 步骤 11：逐步提高速度

- 检查什么：每次只提高一个赛段速度，并记录失败点。
- 成功标准：提高速度后仍不丢线、不打滑、不过冲。
- 失败现象：入弯慢、出弯甩、墙面附着风险增加。
- 只改哪个宏：`GROUND_STRAIGHT_SPEED_MM_S`、`GROUND_CURVE_SPEED_MM_S`、`SHARP_CURVE_SPEED_MM_S`、`TRANSITION_SPEED_MM_S`、`WALL_SPEED_MM_S`。
- 禁止做什么：禁止同时提高风机、速度和转向增益。
## 2026-06-30 补充：五路电磁差速车时序和风机台架

1. 五路电磁 ADC 映射只按去年硬件资源顺序对齐：A/L1=`ADC5`，B/L2=`ADC4`，C/M=`ADC3`，D/R1=`ADC0`，E/R2=`ADC1`。这不是实车线序已验证结论，仍需手持扫线确认。
2. 真实 C251 目标使用两个不同定时器：`Timer1` 只做五路电磁分时采样，`Timer0` 保留 1 ms 时基并每 5 次中断消费完整传感帧运行控制 PID/输出仲裁。不要把 PID 放回采样 ISR。
3. 右直角、环岛、十字共享入口先看 active element count burst。默认 `ELEMENT_SPECIAL_DIRECTION_CONFIGURED=0`，只输出通用特殊元素候选，不猜右直角或环岛方向。
4. 风机 P2.2/PWM2P_3 正常链路仍为 `track_wall_logic -> ctrl_adhesion -> app_output_arbitrate -> bsp_fan_esc_apply`。默认 `FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0`、`WALL_RUN_ENABLE=0`、`SUCTION_HW_VERIFIED=0`、`FAN_BENCH_TEST_ENABLE=0`，不得上墙。
5. 独立风机台架只允许临时打开 `FAN_BENCH_TEST_ENABLE`，且必须固定车体、示波确认 P2.2 脉宽、保持 `WALL_RUN_ENABLE=0`。台架通过不等于上墙通过。

## 2026-06-30 补充：frame 频率修正

1. `Timer1` 是单通道 ADC tick，默认 `SENSOR_ADC_TICK_HZ=1000 Hz`，每 1 ms 只采 A/B/C/D/E 中的一路。
2. 完整五路电磁 frame 默认只有 `SENSOR_FRAME_HZ=200 Hz`，因为五个 tick 才能组成一帧。
3. `Timer0` 控制分频后的 PID 默认 `CONTROL_PID_HZ=200 Hz`，每 5 ms 只消费一个新完整 frame。
4. 没有新 frame 时控制链不重复跑 PI；frame stale 时进入既有丢线/停轮安全路径。
5. 没有示波器实测前，不得宣称控制 ISR 已满足微秒级执行时间指标。
