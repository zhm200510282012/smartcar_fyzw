# Host-SIL Design

## Scope

Host-SIL compiles the current project `App`, `Control`, and `Track` C modules against `sim/host/host_bsp.c`. It does not compile AI8051U register access, official Longqiu driver code, C251 startup, or ISR implementations.

The P0 suite has two explicit build profiles:

- `guard`: wall-state entry disabled while `SUCTION_HW_VERIFIED=0`.
- `logical_wall`: host-only logical wall-state coverage enabled while `SUCTION_HW_VERIFIED=0` and hardware suction output remains zero.

The purpose is to validate deterministic software sequencing and safety arbitration before bench work. It is not evidence that the vehicle can drive, generate suction, or run on a wall.

## Deterministic Clock

The host runner advances `bsp_timebase_on_tick_1ms()` exactly once per simulated millisecond. Each scenario input row is a timestamped sensor snapshot. The runner feeds snapshots into the host BSP, calls `app_scheduler_run_due()`, then records the command package and state telemetry.

## Scheduler Chain

The control-period chain is:

1. input snapshot from BSP
2. sensor freshness and validity
3. electromagnetic line feature update
4. IMU attitude update
5. encoder validity
6. surface-state update
7. transition feature/event update
8. application state machine
9. line control
10. speed control
11. steering control
12. adhesion command selection
13. final safety arbitration
14. command package
15. BSP apply

Final safety arbitration remains the last software decision before BSP apply.

## State Flow

The implemented logical-wall flow is:

`BOOT -> SELF_CHECK -> SENSOR_CALIBRATION -> SAFE_GROUND_READY -> ARMED_GROUND -> transition_candidate_detected -> PRECHARGE -> APPROACH_TRANSITION -> TRANSITION_UP -> WALL_TRACK -> TRANSITION_DOWN -> GROUND_RECOVERY -> FINISHED`

Any active state can be forced into `GROUND_FAULT`, `WALL_FAILSAFE_HOLD`, or `HARD_FAULT` by latched faults. Wall-related faults use `WALL_FAILSAFE_HOLD`; ground-only faults use `GROUND_FAULT`.

If a suction/wall request and transition candidate occur while wall-state capability is disabled, the machine enters `SUCTION_LOCKOUT` instead of any wall-related state.

## Host BSP Boundary

The host BSP reads only scenario inputs and writes only process-local outputs. It does not access device registers. Drive and steering outputs recorded by Host-SIL are software command-package values. Suction physical output remains locked to zero while `SUCTION_HW_VERIFIED == 0`.
