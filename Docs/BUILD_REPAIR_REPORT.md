# C251 链接修复构建报告

最后一次构建动作：

1. 删除历史 OBJ/LST/HEX/MAP/LNP/HTM 等目标产物。
2. `uVision.com -c ... -t AI8051U_FYZW_SAFE`
3. `uVision.com -r ... -t AI8051U_FYZW_SAFE -o build\build_after_c251_link_repair.log`

## 结果

| 项目 | 结果 |
|---|---|
| `ERROR L127` | 已消失 |
| `ERROR L138` | 已消失 |
| Warning | 0 |
| `Target created` 字样 | 未出现；Keil 命令行日志以 `0 Error(s), 0 Warning(s).` 和 HEX 转换完成作为成功输出 |
| HEX 文件 | 已生成：`MDK_Project\OBJ_Out\smartcar_fyzw.hex` |
| 目标文件 | 已生成：`MDK_Project\OBJ_Out\smartcar_fyzw` |
| 当前能否烧录 | HEX 已具备烧录输入条件；实际下载仍需 Keil/STC 下载链路和实物连接确认 |
| 当前是否仍禁止负压输出 | 是，`SUCTION_HW_VERIFIED=0`，所有 `SUCTION_*_NATIVE` 仍为 0 |
| 当前是否仍禁止上墙 | 是，IMU/负压/执行器实物闭环未完成 |

## 本轮链接修复

- `L127` 的根因不是缺少伪函数，而是未使用的官方高层 `LQ_*` 封装被加入目标后引入了 UART/OLED/DMA/TIMER/SPI/ADC/PWM 等依赖闭包。当前 BSP 未调用这些封装，因此从目标移除。
- 按官方测试工程保留 `REMOVEUNUSED`，但没有用它替代缺失符号修复。
- 新增 `Official_Driver_Core` 分组，纳入官方底层 Driver Core 源文件；新增 `Official_Driver_Libs` 分组，纳入 `AI8051U_32_MDU32.LIB`。
- 未加入任何 `Driver\isr\*.c`，避免同一中断向量出现多个拥有者。

## 官方高层封装处理

| 模块 | 是否保留在目标 | 原因 |
|---|---:|---|
| `LQ_MotorServo.c` | 否 | 当前 `BSP` 未调用；负压/舵机/推进输出仍禁能，不能因候选输出而编译高层执行器封装 |
| `LQ_ADC.c` | 否 | 当前 `BSP` 未调用；5LC 通道未闭环 |
| `LQ_LSM6DSR_Hard.c` | 否 | 当前 `bsp_imu.c` 未调用官方 IMU 路径；仅记录 SPI 默认分支，不启用 |
| `LQ_UART.c` | 否 | `bsp_debug_uart.c` 不依赖它；避免引入 DMA/OLED/UART 初始化闭包 |
| `LQ_TIMER.c` | 否 | `bsp_timebase.c` 没有 Timer ISR，未调用官方 Timer 封装 |
| `LQ_Encoder.c` | 否 | 当前 `bsp_encoder.c` 未调用；编码器方向/线数未实测 |

## 最后一次日志摘要

```text
Program Size: data=8.3 edata+hdata=1233 xdata=0 const=744 code=1707
creating hex file from ".\OBJ_Out\smartcar_fyzw"...
".\OBJ_Out\smartcar_fyzw" - 0 Error(s), 0 Warning(s).
```
