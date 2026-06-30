# C251 Compatibility Static Check

Baseline before this repair:

```text
16f8a06 fix: make sensor frames atomic and align PID timing to frame rate
```

Checks performed in this round:

| Check | Result |
|---|---|
| Project self-owned sources do not include `#include "AI8051U.h"` | PASS |
| Project self-owned sources do not define `TMR11_VECTOR` ISR | PASS |
| Project self-owned sources do not call `NVIC_Timer11_Init()` | PASS |
| Project self-owned C ISR entries use Timer0/Timer1 ordinary vectors only | PASS |
| Project self-owned code has no ordinary variable declaration named `bit` | PASS |
| `BSP/bsp_emag.c` no longer returns `emag_sample_t` by value | PASS |
| `app_control_tick_sensor_isr()` real-build local declarations are before executable statements | PASS |
| `BSP/bsp_imu.c` diff hash remained unchanged during this repair | PASS |

Verification commands:

```text
python -m unittest sim.tests.test_c251_compatibility_static
python -m unittest discover -s sim/tests
```

Keil C251 status:

```text
Keil C251 real Clean + Rebuild was not run by Codex.
Do not report 0 Error(s) / 0 Warning(s) from this environment.
The user must run Keil C251 Clean + Rebuild for target AI8051U_FYZW_SAFE and provide the new log if errors remain.
```
