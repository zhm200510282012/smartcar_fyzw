# Real Hardware Blockers

The following blockers remain after the known-pin BSP and full-course Host-SIL repair:

1. `SUCTION_HW_VERIFIED=0`.
2. Real P23 to ESC `PWMIN/GND` wiring has not been closed-loop verified.
3. Real negative-pressure output remains disabled.
4. 5LC ADC channel map is not closed-loop verified on the AI8051U board.
5. IMU bus, orientation, and axis signs are not closed-loop verified.
6. Power sense and UI arm inputs are not verified.
7. Host-SIL is not proof of dynamics, adhesion force, ESC behavior, fan thrust, or track passability.
8. Real wall operation remains forbidden.

Highest-priority hardware verification is still the negative-pressure signal chain with main power isolated: prove the actual ESC input pin, common ground, PWM polarity, frequency, pulse width, safe-off behavior, and no unintended startup before changing `SUCTION_HW_VERIFIED`.
