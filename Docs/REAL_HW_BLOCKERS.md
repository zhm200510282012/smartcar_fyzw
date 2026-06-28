# Real Hardware Blockers

The following blockers remain after the P0 Host-SIL safety-gate repair:

1. `SUCTION_HW_VERIFIED=0`.
2. Real P23 to ESC `PWMIN/GND` wiring has not been closed-loop verified.
3. Real negative-pressure output remains disabled.
4. Real 1 ms hardware timebase ISR is not implemented or verified on the AI8051U target.
5. Host-SIL manual tick is not a real hardware timer.
6. Host-SIL is not proof of dynamics, adhesion force, ESC behavior, fan thrust, or track passability.
7. Real wall operation remains forbidden.

Highest-priority hardware verification is still the negative-pressure signal chain with main power isolated: prove the actual ESC input pin, common ground, PWM polarity, frequency, pulse width, safe-off behavior, and no unintended startup before changing `SUCTION_HW_VERIFIED`.
