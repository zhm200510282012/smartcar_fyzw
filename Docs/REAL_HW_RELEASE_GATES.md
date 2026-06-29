# 真实硬件放行门槛

## 当前结论

当前允许的最高真实测试级别：上电下载、串口观察、主驱/舵机/编码器的离地低功率台架验证。禁止负压输出，禁止上墙，禁止完整赛道闭环实跑。

## 必须保持关闭

| 宏 | 当前值 | 原因 |
| --- | ---: | --- |
| `SUCTION_HW_VERIFIED` | 0 | 负压 ESC 接线、极性、频率、安全关断未闭环 |
| `BOARD_SUCTION_SIGNAL_VERIFIED` | 0 | P23/PWM2N 到 ESC PWMIN 未证明 |
| `BOARD_EMAG_ADC_VERIFIED` | 0 | 5LC 到 AI8051U ADC 通道未闭环 |
| `BOARD_IMU_BUS_VERIFIED` | 0 | LSM6DSR bus、安装方向、正负号未确认 |
| `BOARD_POWER_SENSE_VERIFIED` | 0 | 电源分压和阈值未确认 |
| `BOARD_UI_VERIFIED` | 0 | arm/授权输入未确认 |
| `SUCTION_BENCH_TEST_ENABLE` | 0 | 未进入负压台架授权模式 |

## 分级放行

| 等级 | 允许内容 | 必要证据 |
| --- | --- | --- |
| L0 | Keil 构建、HEX 生成、Host-SIL | 当前已通过 |
| L1 | 串口下载和静态遥测 | UART1/P30/P31 实测稳定 |
| L2 | 离地驱动/舵机/编码器台架 | PWM 波形、方向、机械限位、编码器方向记录 |
| L3 | 地面低速循迹 | 5LC ADC 映射、UI arm、电源采样确认 |
| L4 | 负压主电断开示波测试 | P23 到 ESC PWMIN/GND、50 Hz、1-2 ms、safe-off 无毛刺 |
| L5 | 固定工装负压启停 | ESC 型号、风机方向、电流、温升、急停行为 |
| L6 | 过渡/墙面静态吸附 | IMU 安装方向、吸附力余量、掉电策略 |
| L7 | 上墙动态闭环 | 前面全部证据和逐项回归日志 |

没有 L4-L6 证据前，不得把 `SUCTION_HW_VERIFIED` 改为 1，不得把 Host-SIL logical wall 结果描述为真实上墙成功。
