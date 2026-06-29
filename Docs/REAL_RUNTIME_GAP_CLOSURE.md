# Real Runtime Gap Closure

This pass closes software runtime gaps only. It does not verify real wiring, real motor direction, fan thrust, adhesion, IMU axis, or wall capability.

| Item | Schematic confirmed | Code implemented | Host-SIL verified | Real-car pending |
| --- | ---: | ---: | ---: | ---: |
| Power ADC BSP | no | yes | yes, RUNTIME01 | yes |
| Configurable Arm input | no | yes | yes, RUNTIME02 | yes, change source from bench default before racing |
| Five-channel electromagnetic ADC BSP | no | yes | yes, RUNTIME03 | yes, U9 order and per-channel calibration |
| LSM6DSR SPI BSP | no | yes | yes, RUNTIME04/RUNTIME05 | yes, SPI pins and pitch axis |
| Encoder count/s vs mm/s separation | no | yes | yes, RUNTIME06/RUNTIME07 | yes, counts per rev and wheel circumference |
| Wall approach latch and fan precharge flow | software only | yes | yes, RUNTIME08-RUNTIME10 | yes, wall disabled in C251 defaults |
| Fan ESC arming/failsafe ownership | P2.2 mapped only | yes | yes, RUNTIME11/RUNTIME12 | yes, bench verify ESC before enabling output |
| Wall-cycle flag ownership | software only | yes | yes, RUNTIME13/RUNTIME14 | yes |
| Runtime legacy steering call absence | historical inactive | yes | yes, RUNTIME15 | yes |

Call ownership:

1. App state machine owns boot, self-check, calibration, safe ready, arm, ground track, fault, and finished readiness.
2. `track_wall_logic` owns wall approach, fan precharge, transition up, wall track, cylinder, transition down, recovery, and wall failsafe substate.
3. Output arbitration and safety remain last in the command path.
4. `track_full_course_profile` is telemetry/segment recognition only. It is not route planning.

Safety locks remain:

```c
#define FAN_ESC_PHYSICAL_OUTPUT_ENABLE 0
#define WALL_RUN_ENABLE 0
#define SUCTION_HW_VERIFIED 0
#define ROUTE_PROGRESS_SCRIPT_ENABLE 0
```

Keil status for this pass: blocked on this machine because `uVision.com` / `UV4.exe` was not found. See `build/build_after_real_runtime_gap_closure.log`.
