# One Pass Tuning Manual

Only edit `App/competition_profile.h` for normal tuning.

## Fault Table

| Symptom | Change only this |
| --- | --- |
| Car is left of line but steers farther left | `LINE_DIRECTION_SIGN` or the affected `STEERING_*_SIGN` |
| Two servos move against each other incorrectly | The corresponding `STEERING_*_SIGN` |
| Straight-line snake motion | Lower `GROUND_STEERING_KP` or raise `GROUND_STEERING_KD` |
| Slow turn-in | Raise `GROUND_STEERING_KP` slightly |
| Tail swing after curve exit | Raise `GROUND_STEERING_KD` or lower curve speed |
| Left/right wheel speeds differ | Confirm motor/encoder direction, then tune speed PI conservatively |
| Car keeps going after line loss | Raise `LINE_LOST_QUALITY_MIN` or lower speed before line loss |

## Edit Points

| Need | Edit |
| --- | --- |
| Line polarity | `LINE_DIRECTION_SIGN` |
| Line loss threshold | `LINE_VALID_SUM_MIN`, `LINE_LOST_QUALITY_MIN` |
| Line filtering | `LINE_FILTER_ALPHA` |
| Servo center | `STEERING_LEFT_CENTER_US`, `STEERING_RIGHT_CENTER_US` |
| Servo direction | `STEERING_LEFT_SIGN`, `STEERING_RIGHT_SIGN` |
| Ground base PD | `GROUND_STEERING_KP`, `GROUND_STEERING_KD` |
| Fuzzy self-tuning switch | `FUZZY_ENABLE` |
| Wheel PI | `SPEED_KP`, `SPEED_KI`, `SPEED_OUTPUT_LIMIT`, `SPEED_ACCEL_LIMIT` |
| Speed levels | `GROUND_STRAIGHT_SPEED_MM_S`, `GROUND_CURVE_SPEED_MM_S`, `SHARP_CURVE_SPEED_MM_S` |

## Locked Conditions

Negative pressure remains locked. Wall, cylinder, and transition software paths are not real wall permissions until wiring, suction, IMU, and safety gates are physically verified.
