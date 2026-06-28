# 输入资料审计

审计根目录：`D:\smartcar21`。

标准输入目录：`reference/` 与 `legacy_last_year/` 均已在 `D:\smartcar21` 下发现并审计。

## 已发现资料

| 路径 | 状态 | 用途 |
|---|---|---|
| `D:\smartcar21\第21届智能车竞赛飞檐走壁组比赛赛道制作说明_飞檐走壁赛道-CSDN博客.mhtml` | 已解析 | 本届飞檐走壁赛道制作说明和规则追踪 |
| `reference\2_STC_飞檐走壁组学习套件综合资料\3-AI8051U开源库【请以gitee最新版为准】` | 已发现并复制到 `Drivers\Official_LQ` | AI8051U 官方库、综合测试工程、Keil C251 目标配置 |
| `reference\LQ-AI8051U缩微一体板_MBV2资料\2-原理图\LQ-AI8051驱控一体板学习板原理图_V1.0.pdf` | 已用 `pdftotext` 抽取 | 板级信号名、编码器、LSM6DSR、DRV8701/PWM 证据 |
| `reference\LQ-STC_BLDC_MINI_V3无刷电调打包资料` | 已审计 | ESC 控制口、PWM 高电平时间、共地和故障灯状态 |
| `reference\1-LQ-5LC两路电磁检测板` | 已审计 | 5LC 板资料；PDF 文字只可靠显示“电磁接口/COLQ1/COLQ2/GND”等有限信息 |
| `reference\2_STC_飞檐走壁组学习套件综合资料\6-LQ-LSM6DSR陀螺仪资料\数据手册.pdf` | 已抽取 | LSM6DSR 支持 I2C/SPI、WHO_AM_I、输出寄存器等芯片事实 |
| `reference\Mini编码器` | 已发现 | 编码器资料；当前资源映射主要以 AI8051U 官方 `LQ_Encoder.c` 为准 |
| `legacy_last_year\9.7` | 已复制到 `Legacy_Reference\9.7` | 去年完整工程迁移审计和算法资产 |

## 关键结论

- 官方当前 AI8051U 库 `config.h` 使用 `MAIN_Fosc=40000000L`；去年工程使用 `42000000L`，时钟属于版本差异，当前工程默认跟随官方当前库 40 MHz。
- ESC 手册证明控制信号为 50-300 Hz、1-2 ms 高电平时间，而不是普通高频占空比 PWM。
- 官方 `BLmotor_Init_1()` 提供 AI8051U 一体板 P23 单路无刷 PWM 候选输出；但资料尚未闭环证明当前车负压 ESC 实际接在 P23，因此负压仍默认禁能。
- 去年代码可迁移电磁归一化、差比和差、速度/转向双环、环岛/直角处理、2 ms 定时结构和参数保存思路；不能迁移其硬件假设作为当前事实。
