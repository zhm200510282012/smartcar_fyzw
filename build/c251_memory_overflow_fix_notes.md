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
Keil C251 final link result is not available in Codex.
User must run Clean + Rebuild for target AI8051U_FYZW_SAFE.
The target objective is edata+hdata <= 1536 B while keeping ?STACK at 0x100.
```
