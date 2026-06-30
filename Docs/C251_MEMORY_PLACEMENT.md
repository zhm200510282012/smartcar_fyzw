# C251 内存放置记录

真实 Keil C251 链接失败点：

```text
*** ERROR L107: ADDRESS SPACE OVERFLOW
SPACE: EDATA
SEGMENT: ?STACK
LENGTH: 000100H

Program Size:
data=8.3
edata+hdata=2060
xdata=0
```

本轮不缩小 `?STACK`，不更改 Keil Memory Model，不改 IRAM/XRAM 地址范围。处理方式是把必要的文件级 static/global 持久对象迁移到 AI8051U XDATA。

用户随后真实 Keil Rebuild 已确认链接成功并生成 HEX，但出现：

```text
*** WARNING L56: CONSTANT SEGMENTS IN XDATA AREA
Program Size: data=8.3 edata+hdata=1627 xdata=853 const=8044 code=21564
0 Error(s), 1 Warning(s)
```

该 warning 来自把 `const` 查表数组放入 XDATA。本轮后续修正为：`const` 查表回到默认常量区，继续只迁移非 const 持久状态。

| 对象 | 原文件 | 迁移前存储区 | 迁移后存储区 | 原因 | 是否 ISR 访问 |
| -- | --- | ------ | ------ | -- | --------- |
| `g_app` | `App/main.c` | EDATA/HADATA | XDATA | 主应用上下文包含电磁、IMU、编码器、输出和状态机字段，是最大常驻对象之一 | 是，Timer0 control tick 经绑定指针访问 |
| `g_frame_buffers[3]` | `BSP/bsp_emag.c` | EDATA/HADATA | XDATA | 三缓冲完整电磁 frame payload，保留 front/write/reader 索引在内部数据区 | 是，Timer1 写入，Timer0/control 读取 |
| `g_last_sample` | `BSP/bsp_emag.c` | EDATA/HADATA | XDATA | 完整五路电磁样本缓存 | 是，电磁读取路径访问 |
| `g_track_mode_state` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 赛道模式持久状态 | 是，Timer0 control tick |
| `g_fuzzy_turn_state` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 模糊转向持久状态，含当前增益 | 是，Timer0 control tick |
| `g_line_filter_state` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 线误差低通滤波状态 | 是，Timer0 control tick |
| `g_left_speed_pi` / `g_right_speed_pi` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 左右轮独立 PI 积分和输出状态 | 是，Timer0 control tick |
| `g_wall_logic` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 上墙逻辑持久状态 | 是，Timer0 control tick |
| `g_full_course_profile` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 完整赛段持久状态 | 是，Timer0 control tick |
| `g_adhesion_state` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 风机逻辑状态，物理输出仍由安全开关锁定 | 是，Timer0 control tick |
| `g_stats` | `App/app_control_tick.c` | EDATA/HADATA | XDATA | 控制 tick 统计结构体，非短标志 | 是，Timer0/Timer1 统计更新 |
| `g_last_sample` | `BSP/bsp_encoder.c` | EDATA/HADATA | XDATA | 编码器完整样本缓存 | 是，控制 tick 读取 |
| `g_power_sample` | `BSP/bsp_power.c` | EDATA/HADATA | XDATA | 电源完整样本缓存 | 是，控制 tick 健康检查 |
| `g_last_command` | `BSP/bsp_fan_esc.c` | EDATA/HADATA | XDATA | 风机 ESC 最近命令缓存；物理输出默认仍为 0 | 是，控制 tick 输出仲裁后写入 |
| `g_last_sample` | `Drivers/Board/lsm6dsr_driver.c` | EDATA/HADATA | XDATA | LSM6DSR 原始样本缓存 | 是，IMU BSP 读取路径访问 |
| `g_last_sensor` / `g_last_control` / `g_last_track` / `g_last_health` / `g_last_ui` | `App/app_scheduler.c` | EDATA/HADATA | XDATA | scheduler 持久时间戳合计 20 B，迁移以继续压低 EDATA | 否，主循环访问 |
| `g_drive_*_native` | `BSP/bsp_drive.c` | EDATA/HADATA | XDATA | 最近电机输出缓存 | 是，输出路径访问 |
| `g_steering_*_pulse_us` | `BSP/bsp_steering.c` | EDATA/HADATA | XDATA | 最近舵机脉宽缓存 | 是，输出路径访问 |
| `g_last_command` / `g_last_native_output` | `BSP/bsp_suction.c` | EDATA/HADATA | XDATA | 负压 legacy 输出缓存；安全锁定不变 | 是，安全/输出路径访问 |
| `g_timer_config` | `BSP/bsp_control_timers.c` | EDATA/HADATA | XDATA | Timer 配置遥测结构体，非 ISR 临时变量 | 否，初始化/查询访问 |
| `g_manual_event` / `g_progress_status` | `Track/track_route_event.c` | EDATA/HADATA | XDATA | 路线事件持久状态 | 是，控制 tick 查询 |
| `g_active_mask` 等元素激增持久状态 | `Track/track_features.c` | EDATA/HADATA | XDATA | 元素检测状态机持久状态，迁移不改变阈值/语义 | 是，控制 tick 更新 |

## 保留在 EDATA/HADATA 的对象

| 对象类别 | 文件 | 保留原因 |
|---|---|---|
| `g_front_index` / `g_write_index` / `g_reader_index` | `BSP/bsp_emag.c` | 三缓冲同步索引需短临界区快速访问，且用户要求保持 volatile index 在内部数据区 |
| 单个计数器、标志位、时间戳 | `App/app_control_tick.c`、`BSP/*.c`、`Track/*.c` | 体积小，迁移收益低；保留可减少 XDATA 访问开销 |
| `g_bound_context` 指针 | `App/app_control_tick.c` | 控制 ISR 高频读取的绑定指针，体积小 |
| `BSP/bsp_imu.c` 内 `g_last_attitude` | `BSP/bsp_imu.c` | 该文件存在用户未提交 diff，本轮严格禁止修改 |
| `Control/ctrl_fuzzy_pid.c` 的 `const` 查表 | `Control/ctrl_fuzzy_pid.c` | 保留默认常量区，避免 `WARNING L56: CONSTANT SEGMENTS IN XDATA AREA` |
| `Track/track_strategy.c` 的 `const g_mode_params[]` | `Track/track_strategy.c` | 保留默认常量区，避免 `WARNING L56` |

## 栈与 ISR 审计

- 本轮保持 `?STACK = 0x100`，不缩栈。
- `app_control_tick_control_isr()` 仍只使用有限的局部结构体作为临时输入/输出，不新增大 global 来规避栈。
- Timer0/Timer1 ISR 不调用 `printf`，不使用动态内存，不引入递归。
- 最终 `edata+hdata` 余量只能由用户本机 Keil C251 Clean + Rebuild 的 Program Size 确认。
