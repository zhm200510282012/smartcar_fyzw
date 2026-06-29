# 最终硬件标定检查清单

## 必须先确认的四项物理事实

1. 左右电机正方向：电机 1 为左轮，电机 2 为右轮，正命令让车向前。
2. 左右编码器方向：向前推车时左右速度均为正，计数比例与轮径一致。
3. 五路电磁顺序：A/B/C/D/E 为车头方向从左到右。
4. `line_error` 符号：车偏左/偏右时符号与 `LINE_DIRECTION_SIGN` 配合后能让车回线。

## 电磁

- U9 到 ADC 的 A-E 物理顺序必须实测。
- `BSP/board_emag_map.h` 是物理映射入口。
- `LINE_VALID_SUM_MIN` 和 `LINE_LOST_QUALITY_MIN` 需要在真实赛道线上读取能量后再定。

## 电机和编码器

- 左右 H 桥方向必须离地确认。
- 编码器 A/B 相位必须与前进方向一致。
- 轮径、轮距、编码器比例未实测前，速度闭环只能低功率测试。

## IMU

- 地面静止 pitch 应接近 `IMU_PITCH_OFFSET_CDEG`。
- 上墙方向必须使校正后 pitch 超过 `IMU_WALL_ENTER_CDEG`。
- 下墙回地面必须回到 `GROUND_PITCH_MAX_CDEG` 附近。

## 风机 ESC

- P2.2 必须接 ESC PWMIN，GND 必须共地。
- 首次测试必须离墙、固定车体、低脉宽、可立即断电。
- `FAN_ESC_PHYSICAL_OUTPUT_ENABLE` 默认必须保持 0，台架确认后才允许考虑打开。

## 路线距离

- 上墙前距离：填入 `ROUTE_WALL_APPROACH_DISTANCE_MM`。
- 下墙前距离：填入 `ROUTE_WALL_EXIT_DISTANCE_MM`，用于里程脚本停止上墙入口事件。
- 终点距离：填入 `ROUTE_FINISH_DISTANCE_MM`。
- `ROUTE_PROGRESS_SCRIPT_ENABLE=0` 时这些距离不会自动触发事件。
