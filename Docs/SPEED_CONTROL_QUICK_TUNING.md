# Speed Control Quick Tuning

All user-facing speed items are in `App/competition_profile.h`.

## Current Software Chain

```text
left_target_speed_mm_s  -> left encoder speed  -> left PI -> left PWM/native
right_target_speed_mm_s -> right encoder speed -> right PI -> right PWM/native
```

`drive_command_native` is only the average telemetry value. It must not be used to overwrite left/right native outputs.

## Conservative Defaults

| Item | Default |
| --- | ---: |
| `SPEED_KP` | 32 |
| `SPEED_KI` | 4 |
| `SPEED_OUTPUT_LIMIT` | 260 |
| `SPEED_ACCEL_LIMIT` | 12 |
| `GROUND_STRAIGHT_SPEED_MM_S` | 180 |
| `GROUND_CURVE_SPEED_MM_S` | 140 |
| `SHARP_CURVE_SPEED_MM_S` | 95 |

These are off-ground / low-power starting values only.

## Tuning Order

1. Lift the car so wheels cannot drive the car away.
2. Confirm left and right motor positive direction.
3. Confirm encoder sign and speed increase when each wheel is driven forward.
4. Keep `SPEED_KI` small; tune `SPEED_KP` first.
5. If wheel command jumps too fast, lower `SPEED_ACCEL_LIMIT`.
6. If the two wheels track differently after direction is correct, tune motor-specific mechanics first; then adjust PI conservatively.
