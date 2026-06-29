# C251 Compile Compatibility Fix Notes

本轮只针对用户手工 Keil C251 Rebuild 暴露的当前编译错误与警告做兼容性修复。

## 已处理项

- C48：`App/app_types.h` 在 `HOST_SIL` 下保留本地基础 typedef；真实 C251 分支改为先包含 `../Drivers/Official_LQ/User/DEF.h`，不再重复 typedef `u8/u16/u32/s8/s16/s32`。
- C138：删除项目代码中用于压制未使用参数或返回值的 `(void)xxx;` 写法；编码器未标定路径改为无参 helper；IMU pitch 计算拆为两步。
- C11：`Control/ctrl_adhesion.c` 不再按值返回 `fan_esc_command_t`，改为输出参数形式，并拆分为短小处理函数，逐字段填充输出命令。

## Keil 结果边界

Keil C251 的最终 Rebuild 只能由用户本机完成；
Codex 不得报告“0 Error(s), 0 Warning(s)”；
必须等待用户手动构建日志。

## 安全状态

- `FAN_ESC_PHYSICAL_OUTPUT_ENABLE` 未修改，默认仍为 0。
- `WALL_RUN_ENABLE` 未修改，默认仍为 0。
- `SUCTION_HW_VERIFIED` 未修改，默认仍为 0。
- 未修改电机引脚映射、风机 P2.2 映射、五路电磁 ADC 映射和赛道状态机策略。
