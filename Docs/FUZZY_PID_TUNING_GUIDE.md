# Fuzzy PID Tuning Guide

This is a competition tuning order, not a wall-test permission. Real suction and wall driving remain locked until hardware gates pass.

## Tuning Order

1. Disable fuzzy adjustment conceptually by setting adjustment limits to zero; tune base ground PD only.
2. Tune straight ground `Kp` until line tracking is responsive without oscillation.
3. Tune normal-curve `Kd` for damping and exit stability.
4. Tune sharp curve / omega speed limit, steering offset limit, and steering rate limit.
5. Enable only `Delta Kp` and `Delta Kd`; keep `Ki` off outside stable straight.
6. Try very small straight-only `Ki` after straight and curve PD are stable.
7. Tune transition, wall, cylinder, and seesaw profiles separately on bench or constrained fixtures.

## Parameters To Watch

UART telemetry fields added by this upgrade:

| Field | Purpose |
| --- | --- |
| `track_mode` | selected observable track mode |
| `line_error` | raw line error after line update |
| `line_error_filtered` | low-pass line error used by fuzzy steering |
| `error_rate` | filtered error delta |
| `fuzzy_kp` / `fuzzy_ki` / `fuzzy_kd` | active fixed-point gains |
| `steering_offset_us` | final requested steering offset before arbitration |
| `steering_left_us` / `steering_right_us` | dual-servo pulses after vehicle update |
| `left_speed_target` / `right_speed_target` | mode-selected speed targets |
| `left_speed_measured` / `right_speed_measured` | encoder feedback |

## Safety Rules During Tuning

- Do not tune suction from this module.
- Do not use fuzzy PID to choose a route at a crossing.
- Do not enable real wall tests from Host-SIL results.
- If line quality drops, verify steering returns to center and fuzzy integral resets.
- If a fault or finish state is reached, verify motor command is zero and steering is centered.
