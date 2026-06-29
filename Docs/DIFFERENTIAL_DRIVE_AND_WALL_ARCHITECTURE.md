# 五路电磁差速与上墙软件架构

本轮主控制链已经按无舵机差速车组织。旧舵机文件可以作为历史参考保留，但 `app_scheduler.c`、`app_output_arbitration.c` 和最终 BSP 输出路径不得调用舵机输出。

## 实时链路

```text
A/B/C/D/E 五路电磁
-> ctrl_line_update()
-> line_error / line_error_filtered / line_error_rate / line_quality / line_lost
-> ctrl_fuzzy_turn_update()
-> signed turn_delta_mm_s
-> ctrl_differential_drive_mix()
-> left_target_speed_mm_s / right_target_speed_mm_s
-> ctrl_speed_update()
-> left_drive_command_native / right_drive_command_native
-> app_output_arbitrate()
-> bsp_drive_apply_lr()
```

`drive_command_native` 只保留为左右输出平均值遥测，不再作为真实执行器来源。

## 五路电磁

逻辑通道固定为：

```c
EMAG_A_LEFT,
EMAG_B_LEFT_MID,
EMAG_C_CENTER,
EMAG_D_RIGHT_MID,
EMAG_E_RIGHT
```

权重固定为 `[-2000, -1000, 0, 1000, 2000]`。算法只处理 A-E 逻辑顺序；物理 ADC 线序只能写在 `BSP/board_emag_map.h`。当前映射仍未实车确认，不能把 Host-SIL 注入数据当作真实 5LC 线序证据。

## 差速转向

模糊 PID 的输入仍是：

```text
e  = line_error_filtered
de = line_error_rate
```

输出改为 `turn_delta_mm_s`，再由差速公式生成左右目标速度：

```c
left_target_speed_mm_s = base_speed_mm_s + DIFF_TURN_SIGN * turn_delta_mm_s;
right_target_speed_mm_s = base_speed_mm_s - DIFF_TURN_SIGN * turn_delta_mm_s;
```

正常循迹时单侧不会反转；左右目标速度分别限幅。未 Arm、完成、故障、丢线停车、Kill 时，左右目标速度和 PI 输出归零，同时复位模糊 PID 与速度 PI 状态。

## 风机与上墙边界

`BOARD_FAN_PWM_MAPPED=1` 表示软件资源已经固定为 P2.2/PWM2P_3。`FAN_ESC_PHYSICAL_OUTPUT_ENABLE=0` 表示真实 P2.2 输出保持关闭/安全低电平。`WALL_RUN_ENABLE=0` 表示 C251 实车不会进入真实上墙运行。

Host-SIL 只验证软件状态机、逻辑风机请求和安全仲裁，不证明风机、附着力、IMU 轴向或真实赛道能力。
