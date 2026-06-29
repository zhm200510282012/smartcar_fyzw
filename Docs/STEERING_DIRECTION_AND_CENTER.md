# 舵机路径状态

当前车辆硬件事实为无舵机差速车。`BSP/bsp_steering.c`、`Control/ctrl_vehicle.c`、`Control/ctrl_steering.c` 只作为历史文件保留，不在 `app_scheduler.c`、`app_output_arbitration.c` 或最终 BSP 输出路径中调用。

禁止把以下路径重新作为主控制链：

```text
模糊 PID -> steering_offset_us -> steering_left/right pulse
bsp_steering_apply_pair()
P1.0 / P1.2
PWM1 / PWM2 舵机输出
```

当前主控制链为：

```text
模糊 PID -> turn_delta_mm_s -> 左右目标轮速 -> 左右速度 PI -> bsp_drive_apply_lr()
```

若未来保留遥测字段 `steering_offset_us`、`steering_left_pulse_us`、`steering_right_pulse_us`，它们只能用于兼容旧日志，不得参与真实执行输出。
