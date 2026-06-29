# Steering Direction And Center

All user-facing steering items are in `App/competition_profile.h`.

## Current Software Chain

```text
steering_offset_us from fuzzy/base PD
-> left pulse  = STEERING_LEFT_CENTER_US  + STEERING_LEFT_SIGN  * offset
-> right pulse = STEERING_RIGHT_CENTER_US + STEERING_RIGHT_SIGN * offset
```

The fuzzy PID module only computes `steering_offset_us`. It does not write PWM directly.

## Bench Procedure

1. Power the steering servos with the car lifted or wheels free.
2. Set both centers until the steering linkage is mechanically neutral.
3. Command a small positive offset.
4. If one side moves the wrong way, change only that side's `STEERING_*_SIGN`.
5. Confirm limits do not hit mechanical stops.

Fault, finish, unarmed, kill, and line-lost fault paths return each servo to its own center.
