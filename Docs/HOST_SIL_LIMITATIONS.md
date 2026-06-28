# Host-SIL Limitations

Host-SIL is a software validation layer only.

It does not validate:

- AI8051U timing on the real C251 target.
- A real embedded 1 ms Timer ISR owner or measured scheduler tick on hardware.
- PWM polarity, pulse width on pins, or motor driver response.
- Negative-pressure ESC protocol, polarity, arming, current, temperature, or thrust.
- Encoder electrical direction, count scaling, or wheel slip.
- IMU physical orientation on the assembled board.
- Electromagnetic ADC channel order, calibration, noise, or saturation.
- Battery sag, brownout, EMI, or thermal behavior.
- Real ground driving, wall transition, wall holding, or race readiness.

Result categories must stay separate:

- Code logic pass: C logic and assertions pass on host.
- Host-SIL pass: scenario CSV and summary assertions pass.
- Keil C251 build pass: embedded target compiles and links.
- Real bench pass: not performed by Host-SIL.
- Real wall pass: not performed by Host-SIL.

Current safety status:

- `SUCTION_HW_VERIFIED` remains `0`.
- `BOARD_DRIVE_VERIFIED` remains `0`.
- `BOARD_STEERING_VERIFIED` remains `0`.
- Host-SIL success must not be used to enable nonzero suction output or wall testing.
- The Keil build proves compile/link/HEX generation only; it does not prove the real board scheduler tick is running.
