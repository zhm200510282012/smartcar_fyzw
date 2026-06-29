# Real Car Quick Start

This checklist is for low-power bench and ground tuning. It is not a wall-run approval.

## Four Physical Facts To Confirm First

| Item | What to confirm | Where to change |
| --- | --- | --- |
| A | Left/right motor forward direction | motor wiring or direction handling after bench proof |
| B | Left/right steering center and same/opposite motion | `STEERING_LEFT_CENTER_US`, `STEERING_RIGHT_CENTER_US`, `STEERING_LEFT_SIGN`, `STEERING_RIGHT_SIGN` |
| C | Line left/right sign | `LINE_DIRECTION_SIGN` |
| D | Encoder positive direction and counts per revolution | encoder wiring/BSP evidence; then PI tuning |

Do not run the car before these four facts are confirmed.

## One-Pass Ground Tuning Order

1. Off-ground: confirm both motor directions.
2. Off-ground: confirm both servo centers, limits, and steering direction.
3. Low-speed push: confirm encoder direction and measured speed change.
4. Hand sweep over the electromagnetic line: confirm five channel values and `line_error` sign.
5. Low-speed straight line: keep `FUZZY_ENABLE=0`; confirm no oscillation.
6. Normal curve: after base PD is stable, set `FUZZY_ENABLE=1`.
7. Test sharp curve, omega curve, and loop island separately.

## Hard Safety Notes

- Negative pressure remains locked.
- 5LC ADC, IMU, negative-pressure ESC wiring, and UI wiring are not wall-test evidence until bench verified.
- Host-SIL and Keil build success do not mean the real car has passed the track.
