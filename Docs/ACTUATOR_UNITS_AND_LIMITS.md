# Actuator Units and Limits

## Drive

- Field: `drive_command_native`
- Unit: project-native signed drive command
- Range: `-1000..1000`
- Safe value: `0`
- Current real BSP behavior: forced to `0` while `BOARD_DRIVE_VERIFIED == 0`

## Steering

- Field: `steering_offset_us`
- Unit: microseconds relative to steering center
- Range: `-500..500`
- Safe value: `0`

- Field: `steering_pulse_us`
- Unit: absolute servo PWM pulse width in microseconds
- Center: `1510`
- Range: `1000..2000`
- Safe value: `1510`
- Current real BSP behavior: forced to `1510` while `BOARD_STEERING_VERIFIED == 0`

## Suction

- Field: `suction_cmd.command_native`
- Unit: native suction command for the selected protocol
- Current protocol marker: `SUCTION_PROTOCOL_RC_PULSE_US`
- Current safe output: `0`
- Current precharge/hold/boost/emergency native values: `0`
- Current real BSP behavior: hardware output forced to `0` while `SUCTION_HW_VERIFIED == 0`

Host-SIL records logical suction mode transitions for software coverage. It does not prove real suction output, and it must not be used to enable nonzero suction values.
