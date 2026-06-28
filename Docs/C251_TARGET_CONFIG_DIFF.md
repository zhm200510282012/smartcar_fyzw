# C251 Target 配置差异审计

参考源：`D:\smartcar21\reference\2_STC_飞檐走壁组学习套件综合资料\3-AI8051U开源库【请以gitee最新版为准】\3-Test_Project\3-LQ_AI8051U_Mini_V2_Test_AIO\MDK_Project\LQ_AI8051U_Mini_V2.uvproj`

当前目标：`MDK_Project\smartcar_fyzw.uvproj` / `AI8051U_FYZW_SAFE`

## 必核字段

| 字段 | 官方测试工程 | 当前工程 | 处理 |
|---|---:|---:|---|
| `ToolsetName` | `MCS-251` | `MCS-251` | 一致 |
| `Device` | `AI8051U-32Bit Series` | `AI8051U-32Bit Series` | 一致 |
| `StartupFile` | `LIB\STARTUP251.ASM` | `LIB\STARTUP251.ASM` | 一致 |
| `MemoryModel` | `3` | `3` | 一致，未把它作为 L138 主修复手段 |
| `RomSize` / Code ROM Size | `3` / LARGE | `3` / LARGE | 一致，优先确认 LARGE |
| `Use_Code_Banking` | `0` | `0` | 一致 |
| `UseMemoryFromTarget` | `1` | `1` | 一致 |
| `IROM` | type `1`, start `0xFF0000`, size `0x10000` | type `1`, start `0xFF0000`, size `0x10000` | 一致 |
| `IRAM` | start `0x0000`, size `0x0800` | start `0x0000`, size `0x0800` | 一致 |
| `XRAM` | start `0x10000`, size `0x8000` | start `0x10000`, size `0x8000` | 一致 |
| `Lx51 MiscControls` | `REMOVEUNUSED` | `REMOVEUNUSED` | 保留 |

## 非重点差异

| 字段 | 修改前 | 官方测试工程 | 处理 |
|---|---:|---:|---|
| `C251/AcaOpt` | `1` | `0` | 已同步为 `0` |

## 结论

L138 的 Target 基础字段已经与官方测试工程一致；本次仍按要求重新核验并同步 `AcaOpt`。后续 L138 是否消失只以真实 Clean/Rebuild 日志为准，不用 `REMOVEUNUSED` 替代缺失符号修复。
