# P0 Safety Gate Baseline

Audit date: 2026-06-28

Baseline commit: `2b34a97 feat: add host SIL scenario harness`

Scope inspected:

- `App/app_state_machine.c`
- `App/app_state_machine.h`
- `App/app_scheduler.c`
- `App/app_safety.c`
- `App/app_config.h`
- `BSP/bsp_suction.c`
- `BSP/bsp_suction.h`
- `BSP/board_map.h`
- `Track/`
- `sim/host/sil_runner.c`
- `sim/host/`
- `sim/scenarios/`
- `sim/tests/`
- `Docs/`

## Findings

1. Wall-related application states at baseline:
   - `APP_STATE_SUCTION_PRECHARGE`
   - `APP_STATE_APPROACH_TRANSITION`
   - `APP_STATE_TRANSITION_UP`
   - `APP_STATE_WALL_TRACK`
   - `APP_STATE_TRANSITION_DOWN`
   - `APP_STATE_WALL_FAILSAFE_HOLD`

2. With `SUCTION_HW_VERIFIED=0`, these states are currently reachable in Host-SIL.
   - Baseline command: `python sim/scripts/run_sil.py`
   - Baseline S10 state sequence:
     `BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK`
   - This is the P0 safety-gate defect: S10 is named suction-unverified but still reaches `WALL_TRACK`.

3. Baseline precharge condition:
   - `APP_STATE_ARMED_GROUND` currently enters `APP_STATE_SUCTION_PRECHARGE` when `manual_suction_authorize && sensors_ready(ctx) && transition_up_observed(ctx)` is true.
   - This means the command precharge stage is triggered after an upward transition or wall surface has already been observed.

4. Baseline `transition_up_observed()` meaning:
   - It returns true for `SURFACE_TRANSITION_UP` or `SURFACE_WALL`.
   - It is an observed posture/surface result, not a pre-transition candidate event.

5. Host-SIL time progression:
   - `sim/host/sil_runner.c` reads timestamped CSV rows.
   - It advances `bsp_timebase_on_tick_1ms()` in a loop until the input timestamp is reached.
   - This is a deterministic host-side manual tick only. It is not a real AI8051U hardware Timer ISR.

6. Host-SIL logical suction request path:
   - `Control/ctrl_adhesion.c` writes `ctx->suction_cmd.command_native` from suction constants according to app state.
   - At baseline, all `SUCTION_*_NATIVE` constants are zero, so the Host-SIL logical suction request remains zero even in wall states.

7. Host-SIL hardware native suction output lock:
   - `sim/host/host_bsp.c::bsp_suction_apply()` stores the command but, while `SUCTION_HW_VERIFIED == 0`, always writes `g_suction_native_output = SUCTION_SAFE_OFF_NATIVE`.
   - Current `SUCTION_SAFE_OFF_NATIVE` is zero.
   - Baseline S10 max hardware output is zero, but this does not justify entering wall states.

8. Real C251 target 1 ms ISR status:
   - `BSP/bsp_timebase.c` provides `bsp_timebase_on_tick_1ms()`.
   - The current target has no owned Timer ISR calling it.
   - `Docs/ISR_OWNERSHIP_TABLE.md` records Timer vectors as disabled/unowned.
   - Host-SIL manual ticking must not be described as a verified real 1 ms hardware timebase.
