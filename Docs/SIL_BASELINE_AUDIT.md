# Host-SIL Baseline Audit

Audit date: 2026-06-28

Scope read before changes: `README.md`, `Docs/`, `App/`, `BSP/`, `Control/`, `Track/`, `MDK_Project/`, `build/`, `tools/`.

## Required Findings

1. `bsp_timebase_now_ms()` time source:
   - File/function: `BSP/bsp_timebase.c::bsp_timebase_now_ms()`.
   - The returned value is `static volatile u32 g_ms`.
   - `g_ms` increments only in `BSP/bsp_timebase.c::bsp_timebase_on_tick_1ms()`.
   - Current `App/main.c` calls `bsp_timebase_now_ms()` but no real ISR or loop calls `bsp_timebase_on_tick_1ms()`.

2. Unique 1 ms timer ISR or equivalent tick entry:
   - File/function: `BSP/bsp_timebase.c::bsp_timebase_on_tick_1ms()`.
   - There is an equivalent tick entry, but no Keil ISR owner is currently defined.
   - `Docs/ISR_OWNERSHIP_TABLE.md` records Timer vectors as unowned and disabled.

3. Scheduler closed-loop chain:
   - File/function: `App/app_scheduler.c::app_scheduler_run_due()`.
   - Current scheduler samples `bsp_emag_read()`, `bsp_imu_read()`, `bsp_encoder_read()`.
   - It runs `ctrl_line_update()`, `ctrl_profile_speed_limit()`, `ctrl_adhesion_update()`, `app_safety_apply_profile()`, then `bsp_drive_apply()`, `bsp_steering_apply()`, `bsp_suction_apply()`.
   - Missing from the real chain: explicit input freshness checks for every sensor, IMU attitude update through `ctrl_attitude_update()`, encoder-speed-based drive command generation through `ctrl_speed_command()`, steering command generation through `ctrl_steering_command()`, surface estimation through `track_surface_state_update()` or `ctrl_transition_estimate_surface()`, and vehicle command assembly through `ctrl_vehicle_update()`.
   - `App/app_state_machine.c::app_state_machine_step()` currently runs in a later 20 ms task, after the control outputs have already been applied in the 2 ms control task.
   - Safety exists, but because state-machine and normal control are split by periods, the current ordering is not yet the requested single closed-loop sequence.

4. Current command units:
   - `app_context_t::drive_cmd` is a signed native command, nominally `-1000..1000`, with safe value `DRIVE_SAFE_ZERO`.
   - `app_context_t::steering_cmd` is ambiguous. `BSP/bsp_steering.c::bsp_steering_init()` stores `STEERING_SAFE_CENTER=1510`, but `bsp_steering_apply()` clamps incoming commands using `STEERING_LIMIT_ABS=1000`, which is a relative-offset style limit.
   - `app_context_t::suction_cmd.command_native` is a native suction command. Current constants are all `0` because `SUCTION_HW_VERIFIED=0`.

5. Steering command type:
   - `Control/ctrl_steering.c::ctrl_steering_command()` returns `line_error / 4`, a relative offset.
   - `BSP/bsp_steering.c::bsp_steering_apply()` stores the value directly as though it were the final command.
   - Current code mixes absolute pulse center (`1510 us`) with relative offset output.

6. Steering unit conflict:
   - Conflict exists.
   - `STEERING_SAFE_CENTER=1510` is an absolute pulse width, while `STEERING_LIMIT_ABS=1000` is used as a relative clamp.
   - If `BOARD_STEERING_VERIFIED` were enabled without fixing this, the safe center value would be clipped by the wrong limit path.

7. State-machine support:
   - Current states are declared in `App/app_types.h::app_state_t`.
   - Current implementation in `App/app_state_machine.c::app_state_machine_step()` supports automatic `BOOT -> SELF_TEST -> SENSOR_CALIBRATION -> SAFE_GROUND_READY`, manual arming to `ARMED_GROUND`, optional `SUCTION_PRECHARGE`, and partial transition timeout handling.
   - It does not yet implement a complete `APPROACH_TRANSITION -> TRANSITION_UP -> WALL_TRACK -> TRANSITION_DOWN -> GROUND_RECOVERY -> FINISHED` flow.
   - There is no `FINISHED` state in `app_state_t` yet.

8. Current safe stubs:
   - `BSP/bsp_emag.c::bsp_emag_read()` returns invalid zero data.
   - `BSP/bsp_imu.c::bsp_imu_read()` returns invalid zero attitude.
   - `BSP/bsp_encoder.c::bsp_encoder_read()` returns invalid zero encoder data.
   - `BSP/bsp_power.c::bsp_power_is_ok()` returns `APP_FALSE`.
   - `BSP/bsp_ui.c::bsp_ui_manual_arm_requested()` and `bsp_ui_suction_authorized()` return `APP_FALSE`.
   - These are safety stubs for the embedded target and are not valid simulated sensor results.

9. Suction hardware lock:
   - `App/app_config.h` keeps `SUCTION_HW_VERIFIED=0`.
   - `BSP/bsp_suction.c::bsp_suction_apply()` forces `SUCTION_OFF`, `SUCTION_SAFE_OFF_NATIVE`, `armed=APP_FALSE`, `hw_verified=APP_FALSE`, and writes only safe-off when `SUCTION_HW_VERIFIED == 0`.
   - All `SUCTION_*_NATIVE` constants remain `0`.

## Baseline Blockers For Host-SIL

- No host C compiler is currently discoverable on this machine (`gcc`, `clang`, `cl`, `zig`, `cc` all unavailable).
- The scheduler does not yet drive a complete deterministic control chain.
- The embedded BSP is intentionally safe-stubbed; Host-SIL must provide a separate Host BSP and must not change these embedded safety stubs into fake hardware.
- Steering command units need to be split into relative `steering_offset_us` and absolute `steering_pulse_us`.
- The state machine needs explicit transition predicates, debounce/timeout behavior, and a final state.
