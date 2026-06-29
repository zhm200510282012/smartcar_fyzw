# 飞檐走壁差速车最终软件流程

## 车体模型

当前主路径固定为五路电磁差速车：

- 电磁 A/B/C/D/E：车头方向从左到右。
- 电机 1：左轮。
- 电机 2：右轮。
- 风机 ESC：P2.2 / PWM2P_3，50 Hz，1-2 ms 脉宽协议。
- 历史舵机路径：`BSP/bsp_steering.*`、`Control/ctrl_steering.*`、`Control/ctrl_vehicle.*` 标记为 historical / inactive，不被 scheduler、output arbitration 或 Keil 主路径调用。

## 控制链

真实 scheduler 中执行的链路为：

```text
五路电磁 A-E
-> ctrl_line_update 五路加权重心
-> line_error_filtered / line_error_rate / line_quality
-> ctrl_fuzzy_turn_update 基础 PD 或模糊自整定
-> turn_delta_mm_s
-> ctrl_differential_drive_mix
-> left/right target speed
-> ctrl_speed_update_pair 左右独立 PI
-> bsp_drive_apply_lr 左右 H 桥输出
```

风机和墙面链路为：

```text
route_event / IMU pitch / line_lost / adhesion_risk
-> track_wall_logic_update
-> fan_esc_state + speed_limit + drive_allowed
-> ctrl_adhesion_update
-> app_output_arbitrate
-> bsp_fan_esc_apply
```

`FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0` 时，逻辑 request_us 可以变化，真实 P2.2 输出仍强制为 0。

## 全程赛段

`Track/track_full_course_profile.*` 记录当前赛段：

```text
START
-> GROUND_STRAIGHT
-> NORMAL_CURVE
-> SHARP_CURVE
-> CROSSING
-> OMEGA
-> HEX_LOOP
-> WALL_APPROACH
-> FAN_PRECHARGE
-> TRANSITION_UP
-> WALL_TRACK
-> CYLINDER_TRACK
-> TRANSITION_DOWN
-> GROUND_RECOVERY
-> GROUND_TRACK 或 FINISH
```

地面直线、普通弯、急弯主要由电磁误差和误差变化率识别。十字、Omega、环岛、上墙入口和终点由 `route_event` 或未来路线表决定，不在代码中猜测真实赛道唯一顺序。

## route_event 来源

集中配置在 `App/competition_profile.h`：

```c
#define ROUTE_EVENT_SOURCE_DEFAULT          ROUTE_EVENT_SOURCE_NONE
#define ROUTE_PROGRESS_SCRIPT_ENABLE        0
#define ROUTE_WALL_APPROACH_DISTANCE_MM     0L
#define ROUTE_WALL_EXIT_DISTANCE_MM         0L
#define ROUTE_FINISH_DISTANCE_MM            0L
```

- `HOST_SIL`：完整路线测试注入。
- `MANUAL_INJECT`：未来串口/按键调试入口，不依赖未确认 UI。
- `PROGRESS_SCRIPT`：按编码器累计距离触发事件，默认关闭。

## Host-SIL 与真实车区别

Host-SIL 证明控制逻辑、状态时序和安全门按软件规则运行。它不证明电磁 ADC 顺序、IMU 轴向、编码器方向、风机吸力或 ESC 接线已经正确。

Keil 0 error/0 warning 证明 C251 可构建。它不证明电机、风机、IMU 或赛道硬件方向正确。
