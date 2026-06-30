# 控制时序架构

当前实时链路明确区分三种频率：

| 概念 | 宏 | 默认值 | 含义 |
|---|---|---:|---|
| 单通道 ADC tick | `SENSOR_ADC_TICK_HZ` | 1000 Hz | Timer1 每次只采一个电磁 ADC 通道 |
| 完整五路 frame | `SENSOR_FRAME_HZ` | 200 Hz | A/B/C/D/E 五个 tick 后才发布一个完整 frame |
| 控制 PID | `CONTROL_PID_HZ` | 200 Hz | Timer11 消费新完整 frame 后运行一次完整控制链 |

约束：

```c
SENSOR_FRAME_HZ = SENSOR_ADC_TICK_HZ / EMAG_CHANNEL_COUNT
CONTROL_PID_HZ <= SENSOR_FRAME_HZ
```

## 真实目标路径

```text
Timer1, 1 ms
-> app_control_tick_sensor_isr()
-> bsp_emag_sensor_tick()
-> 每 tick 只采 A/B/C/D/E 中一个通道
-> 五个 tick 后发布一个完整 frame sequence

Timer11, 5 ms
-> app_control_tick_control_isr()
-> 只在拿到新完整 sequence 时运行完整控制链
```

完整控制链包括：

```text
line
-> filter/rate/quality
-> element burst feature
-> route/state
-> basic PD or fuzzy turn
-> differential mix
-> left/right speed PI
-> wall/fan logic
-> output arbitration
-> drive/fan BSP
```

## 无新 frame 行为

如果 Timer11 本次没有新完整 frame，或 sequence 与上次已消费 frame 相同，则不重复运行：

```text
ctrl_line_update
ctrl_fuzzy_turn_update
ctrl_speed_update_pair
ctrl_differential_drive_mix
track_wall_logic_update
app_output_arbitrate
```

此时保持上一周期输出。若 frame 超过 `SENSOR_STALE_TIMEOUT_MS`，则进入既有 line_lost/安全停轮路径，不允许保持旧高速输出。

## 三缓冲发布

`BSP/bsp_emag.c` 使用 3 个 frame buffer：

```text
front  = 最新已发布完整 frame
reader = control ISR 正在字段级复制的 frame
write  = sensor ISR 正在写入的下一帧
```

sensor ISR 发布 frame 时只切换 `g_front_index`，随后选择一个既不是 front 也不是 reader 的 buffer 作为下一帧 write。control ISR 只在极短临界区锁定 front index 到 reader，随后恢复 Timer1 中断并字段级复制。

## Host-SIL 路径

Host-SIL scheduler 按真实频率模拟：

```text
sensor tick: 每 1 ms
control tick: 每 5 ms
```

`timing_element_fan` runner 使用真实 `BSP/bsp_emag.c` 扫描路径验证：五个 sensor tick 才产生一个新 sequence，同一 sequence 只触发一次完整 PID 链，三缓冲不会产生 A/B 为新帧而 C/D/E 为旧帧的撕裂 frame。
