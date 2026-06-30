# 控制时序架构

当前真实控制链拆成两个周期：

| 周期 | 默认频率 | 入口 | 职责 | 禁止事项 |
|---|---:|---|---|---|
| 传感 tick | `SENSOR_FRAME_HZ=1000 Hz` | `Timer1 -> app_control_tick_sensor_isr()` | 五路电磁按 A/B/C/D/E 分时采样，提交完整 frame | 不调用 PID，不写电机，不写风机 |
| 控制 tick | `CONTROL_PID_HZ=500 Hz` | `Timer11 -> app_control_tick_control_isr()` | 消费完整电磁 frame，读编码器/IMU，运行 line、状态机、模糊转向、双轮 PI、风机逻辑、输出仲裁 | 不直接轮询五路 ADC |
| 低频 scheduler | `app_scheduler_run_due()` | 主循环 | UI、健康检查、低频编排 | 非 `HOST_SIL` 不重复调用快速 sensor/control tick |

## 真实目标路径

```text
Timer1 ISR
-> bsp_emag_sensor_tick()
-> 完整五路 frame 提交

Timer11 ISR
-> 读取最新完整 frame
-> ctrl_line_update()
-> track_features_update_elements()
-> track_route_profile_select()
-> ctrl_fuzzy_turn_update()
-> ctrl_speed_update_pair()
-> ctrl_adhesion_update()
-> app_output_arbitrate()
-> bsp_drive_apply_lr()
-> bsp_fan_esc_apply()
```

`Timer11` 中如果 frame 缺失或超过 `SENSOR_STALE_TIMEOUT_MS`，电磁样本被标为 invalid/line_lost，输出链按现有安全策略归零或保持禁能。

## Host-SIL 路径

Host-SIL 没有真实硬件中断，仍通过 scheduler 在固定时间推进两类 tick。新增测试 `timing_element_fan` 验证：

- sensor tick 不调用双轮 PI；
- control tick 每次只调用一次 `ctrl_speed_update_pair()`；
- control tick 不直接标记 ADC 读取；
- stale sensor frame 会进入 line_lost；
- Host-SIL 通过不代表实车时序和中断延迟已验证。
