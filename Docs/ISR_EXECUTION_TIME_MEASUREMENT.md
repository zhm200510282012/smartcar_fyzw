# ISR 执行时间测量说明

当前代码没有已验证的高精度自由运行计时器，因此不再声称存在真实微秒级 ISR 执行时间统计。

## 当前代码内统计

`app_control_tick_stats_t` 只保留逻辑迟到/缺帧计数：

```text
sensor_frame_stale_count
control_no_new_frame_count
control_skipped_duplicate_frame_count
control_deadline_miss_count
```

这些计数说明 frame 是否迟到、重复或缺失，不等同于 ISR 高电平宽度，也不代表控制 ISR 已满足某个微秒阈值。

## 示波器测量 hook

新增 no-op 接口：

```c
void bsp_timing_scope_sensor_enter(void);
void bsp_timing_scope_sensor_exit(void);
void bsp_timing_scope_control_enter(void);
void bsp_timing_scope_control_exit(void);
```

默认：

```c
#define BSP_TIMING_SCOPE_ENABLE 0
```

此状态下不会写任何 GPIO，不会占用未确认引脚。

## 后续实测流程

只有用户确认一个空闲示波器调试引脚后，才允许把 `BSP_TIMING_SCOPE_ENABLE` 改为 `1` 并在 `BSP/bsp_timing_scope.c` 内实现对应 GPIO 写入。

测量方式：

```text
Timer1 ISR 入口拉高 SENSOR scope，退出拉低；
Timer0 控制分频 tick 入口拉高 CONTROL scope，退出拉低；
示波器测 CONTROL 高电平宽度。
```

当前 `CONTROL_PID_HZ=200 Hz`，周期为 5 ms。后续若要把完整 frame 提升到 500 Hz 或 1000 Hz，必须先实测单次 `Get_ADCResult` 和控制 ISR 高电平宽度，并保持：

```text
CONTROL_PID_HZ <= SENSOR_FRAME_HZ
```

没有示波器实测前，不得宣称“控制 ISR 已满足 800 us”或类似微秒级结论。
