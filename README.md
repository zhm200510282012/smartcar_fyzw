# smartcar_fyzw

AI8051U 飞檐走壁组整车安全重构工程，根目录：`D:\smartcar21\smartcar_fyzw`。

- Keil 工程：`MDK_Project\smartcar_fyzw.uvproj`
- 官方库副本：`Drivers\Official_LQ`
- 去年代码副本：`Legacy_Reference\9.7`
- 关键审计：`Docs\NEGATIVE_PRESSURE_HW_AUDIT.md`

当前负压 ESC 协议证据充分，但 P23 到 ESC 的实车接线未闭环，所以 `SUCTION_HW_VERIFIED=0`，不会输出非零负压命令。
