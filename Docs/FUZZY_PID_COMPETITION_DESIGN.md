# Fuzzy PID Competition Design

## Scope

This upgrade adds a lightweight fixed-point fuzzy self-tuning PID layer for steering only.

Control chain:

1. five-channel electromagnetic sample
2. `line_error`
3. `line_error_filtered`
4. `error_rate`
5. observable track-mode selection
6. fuzzy self-tuning `Kp/Ki/Kd`
7. conventional PID steering offset
8. dual steering servo pulse output through existing final arbitration

The speed loop remains the existing encoder-based command path. Track mode only selects target speed and steering limits.

## Fixed-Point Scaling

Inputs are normalized to `[-1000, +1000]`:

| Signal | Macro | Meaning |
| --- | ---: | --- |
| `e` | `FUZZY_E_SCALE = 1000` | filtered line error normalization scale |
| `de` | `FUZZY_DE_SCALE = 1000` | line-error-rate normalization scale |
| membership weight | `FUZZY_WEIGHT_SCALE = 1000` | 1000 means full membership |
| PID output divisor | `FUZZY_PID_OUTPUT_SCALE = 1000` | converts fixed-point PID sum to servo microseconds |

No `float`, `double`, dynamic allocation, recursion, or large rule network is used.

## Safety Integration

Fuzzy steering runs before `app_output_arbitrate()`. It cannot override final safety output.

Fuzzy PID state is reset when:

- outputs are not allowed;
- `TRACK_MODE_LINE_LOST` is selected;
- line quality is below `TRACK_LINE_LOST_QUALITY_MIN`;
- state is `FINISHED`, `GROUND_FAULT`, `SUCTION_LOCKOUT`, `WALL_FAILSAFE_HOLD`, or `HARD_FAULT`;
- the active track mode changes.

`SUCTION_HW_VERIFIED` remains `0`; real suction native output remains `0`.

## Files

| File | Role |
| --- | --- |
| `Control/ctrl_fuzzy_pid.c` | membership functions, 5x5 rules, weighted average, gain limits |
| `Control/ctrl_fuzzy_steering.c` | conventional fixed-point PID steering using fuzzy-updated gains |
| `Track/track_route_profile.c` | generic observable track mode selector with debounce/dwell |
| `Track/track_strategy.c` | per-mode base PID, steering limits, rate limits, speed targets |
| `App/app_scheduler.c` | integrates line terms, mode selection, fuzzy steering, safety reset |
