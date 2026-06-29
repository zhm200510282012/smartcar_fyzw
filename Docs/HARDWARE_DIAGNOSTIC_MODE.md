# Hardware Diagnostic Mode

`App/app_hardware_diagnostic.c` provides a no-drive diagnostic pass for confirming the real sensor chain before any wall attempt.

| Diagnostic item | Schematic confirmed | Code implemented | Host-SIL verified | Real-car pending |
| --- | ---: | ---: | ---: | ---: |
| Power raw/filtered/status | no | yes | RUNTIME01 | yes |
| Arm request | no | yes | RUNTIME02 | yes |
| A-E electromagnetic ADC raw values | no | yes | RUNTIME03 | yes |
| IMU WHO_AM_I/SPI/raw accel/raw gyro/pitch | no | yes | RUNTIME04/RUNTIME05 | yes |
| Encoder delta counts | partially mapped | yes | RUNTIME06 | yes |
| Route progress status | software only | yes | RUNTIME07 | yes |

Diagnostic mode must not:

- output left or right motor PWM;
- output fan PWM;
- enter wall states;
- unlock negative pressure.

The diagnostic function only samples BSP state and writes one UART line. It is intentionally not enabled automatically from `main`.

Before any real run, use this mode to record:

1. battery ADC raw and filtered values;
2. GO/Arm response;
3. five electromagnetic channels when sweeping across the wire;
4. IMU `WHO_AM_I`, SPI status, accel/gyro raw values, and configured pitch sign;
5. left/right encoder delta count signs.
