# Electromagnetic Calibration

All user-facing line tuning items are in `App/competition_profile.h`.

## Current ADC Resource Order

The current mapping layer follows last year's hardware resource order only:

```text
A / L1 = ADC5
B / L2 = ADC4
C / M  = ADC3
D / R1 = ADC0
E / R2 = ADC1
```

This is not a bench-verified line-order claim. `BOARD_EMAG_LINE_ORDER_VERIFIED` remains `0`, and real-car tuning must still confirm that A/B/C/D/E correspond to left-to-right physical sensors.

## Current Software Chain

Five normalized electromagnetic channels are converted to a weighted centroid:

```text
weights = [-2, -1, 0, +1, +2]
line_error = LINE_DIRECTION_SIGN * sum(weight[i] * norm[i]) * LINE_WEIGHT_SCALE / sum(norm[i])
```

The firmware requires `channel_count == 5` and `sum(norm) >= LINE_VALID_SUM_MIN`. Otherwise `line_lost` is true and the control chain resets the filtered error.

## Items To Confirm

1. Sweep the car by hand from left of the line to right of the line.
2. Watch `norm[0..4]`, `line_error`, `line_error_filtered`, `line_error_rate`, and `line_lost`.
3. If left/right sign is reversed, change only `LINE_DIRECTION_SIGN`.
4. If weak signal is accepted as valid, raise `LINE_VALID_SUM_MIN` or `LINE_LOST_QUALITY_MIN`.
5. If steering reacts late, lower `LINE_FILTER_ALPHA`; if steering is noisy, raise it.

The real BSP keeps the mapping marked unverified until the real ADC channel order is confirmed on the car.
