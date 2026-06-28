# 构建报告

## Keil C251

未执行 Keil C251 真实编译。本机 PATH 未发现 `UV4.exe` 或 `C251.exe`。工程文件 `MDK_Project/smartcar_fyzw.uvproj` 已基于龙邱官方 `LQ_AI8051U_32Bit_LIB.uvproj` 的 AI8051U-32Bit / MCS-251 目标配置生成，并替换为本工程 App/BSP/Control/Track 分组。

## 已执行静态检查

```powershell
python D:\smartcar21\smartcar_fyzw\tools\check_project.py
python D:\smartcar21\smartcar_fyzw\tools\check_includes.py
python D:\smartcar21\smartcar_fyzw\tools\check_uvproj_files.py
```

本轮实际结果：

```text
PASS: project integrity checks passed
PASS: local include paths resolve
PASS: uvproj file paths exist
```

结果已写入 `build\build.log`。

## 说明

- 静态检查通过不等于 Keil 编译通过。
- 官方库已复制到 `Drivers\Official_LQ`，但危险执行器适配默认禁能。
- Keil 上首次构建前，应先打开官方原始工程确认 C251 环境和 AI8051U 头文件安装正常。
