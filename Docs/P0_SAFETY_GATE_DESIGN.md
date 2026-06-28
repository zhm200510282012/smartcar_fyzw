# P0 Safety Gate Design

## Root Defect

The previous Host-SIL state machine allowed `S10_suction_unverified` to enter:

`BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK`

This happened while `SUCTION_HW_VERIFIED=0`. The state machine also started command precharge from `transition_up_observed()`, which means precharge began after upward posture or wall surface had already been observed.

## Safety Gate

`App/app_build_profile.h` separates physical suction capability from Host-SIL logical capability:

- C251 firmware: `APP_WALL_STATE_CAPABLE == SUCTION_HW_VERIFIED`
- Host-SIL Guard Profile: `APP_WALL_STATE_CAPABLE == 0`
- Host-SIL Logical Wall Profile: `APP_WALL_STATE_CAPABLE == 1`, while `SUCTION_HW_VERIFIED` remains `0`

Compile-time guards reject:

- Host-SIL logical wall macros in non-Host-SIL builds.
- Host-SIL builds without an explicit profile.
- Logical suction availability outside the explicit Host-SIL Logical Wall Profile.
- C251 wall capability that differs from `SUCTION_HW_VERIFIED`.

## Lockout State

`APP_STATE_SUCTION_LOCKOUT` is entered when:

`manual_suction_authorize && sensors_ready && transition_candidate_detected && APP_WALL_STATE_CAPABLE == 0`

Final safety arbitration forces:

- `drive_command_native = 0`
- `steering_offset_us = 0`
- `steering_pulse_us = STEERING_SAFE_CENTER_US`
- `suction_cmd.mode = SUCTION_OFF`
- `suction_cmd.command_native = 0`
- hardware suction output remains `0`

This is an expected safety refusal state, not a post-fault wall hold.

## Candidate Versus Observed Transition

`transition_candidate_detected` means a pre-wall candidate event exists before the upward pitch/surface change. In production C251 firmware it defaults to false because no hardware-verified candidate source exists.

`transition_up_observed` means the vehicle is already observing transition-up or wall surface state.

The corrected sequence is:

`ARMED_GROUND -> transition_candidate_detected -> SUCTION_PRECHARGE -> dwell -> APPROACH_TRANSITION -> transition_up_observed -> TRANSITION_UP -> wall_observed -> WALL_TRACK`

Host-SIL may inject `transition_candidate` from CSV. It must not use `SURFACE_WALL` as the precharge trigger.
