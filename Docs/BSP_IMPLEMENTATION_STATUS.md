# BSP Implementation Status

| BSP | Schematic confirmed | Code implemented | Host-SIL verified | Real-car pending |
| --- | ---: | ---: | ---: | ---: |
| `BSP/bsp_power.c` | no | yes, official ADC path plus raw/filtered telemetry | RUNTIME01 | battery divider channel and threshold |
| `BSP/bsp_ui.c` | no | yes, bench/GO/UART source layer | RUNTIME02 | actual GO/UART arm source |
| `BSP/bsp_emag.c` | no | yes, five real ADC reads and A-E logical mapping | RUNTIME03 | U9 order, offset/gain/invert |
| `BSP/bsp_imu.c` + `Drivers/Board/lsm6dsr_driver.c` | no | yes, SPI WHO_AM_I and raw accel/gyro path | RUNTIME04/RUNTIME05 | SPI wiring and pitch axis |
| `BSP/bsp_encoder.c` | partially mapped | yes, count delta/count/s plus guarded mm conversion | RUNTIME06/RUNTIME07 | counts/rev and wheel circumference |
| `BSP/bsp_fan_esc.c` | P2.2 mapped only | yes, physical active/applied callbacks | RUNTIME11/RUNTIME12 | ESC bench arm and fan response |

Important distinctions:

- `BOARD_FAN_ESC_SIGNAL_MAPPED=1` means the software signal path is mapped to P2.2/PWM2P_3.
- `BOARD_FAN_ESC_BENCH_VERIFIED=0` means the physical ESC/fan has not been bench verified.
- `POWER_GUARD_ENABLE=0` permits low-risk ground bench debugging, but telemetry must be treated as `POWER_UNCALIBRATED`.
- `valid` in the electromagnetic BSP means the five ADC conversions completed. Line quality and line loss remain control-layer decisions.

No new fake functions were added to satisfy links.
