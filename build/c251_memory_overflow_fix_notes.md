# C251 Memory Overflow Fix Notes

Issue from user Keil C251 rebuild:

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

Fix direction:

```text
Do not shrink ?STACK.
Do not change Keil Memory Model.
Do not change IRAM/XRAM address ranges.
Move persistent static/global state and large lookup arrays to XDATA using APP_XDATA.
```

Important retained safety state:

```c
#define FAN_ESC_PHYSICAL_OUTPUT_ENABLE 0
#define WALL_RUN_ENABLE                0
#define SUCTION_HW_VERIFIED            0
#define FAN_BENCH_TEST_ENABLE          0
```

Keil status:

```text
User Keil C251 rebuild after commit 2f38fdf:
linking...
*** WARNING L56: CONSTANT SEGMENTS IN XDATA AREA
Program Size: data=8.3 edata+hdata=1627 xdata=853 const=8044 code=21564
creating hex file from ".\OBJ_Out\smartcar_fyzw"...
".\OBJ_Out\smartcar_fyzw" - 0 Error(s), 1 Warning(s).
```

Follow-up fix:

```text
Removed APP_XDATA from const lookup tables to address L56.
Moved additional non-const persistent state to XDATA to reduce edata+hdata further.
The target objective remains edata+hdata <= 1536 B while keeping ?STACK at 0x100.
Final Program Size must be confirmed by the next user Keil Clean + Rebuild.
```
