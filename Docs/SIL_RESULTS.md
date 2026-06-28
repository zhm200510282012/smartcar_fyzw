# Host-SIL Results

These results are Host-SIL software results only. They do not verify bench hardware, suction polarity, real drive output, or wall operation.

| scenario | pass | actual state sequence | max speed cmd | max steering cmd | max logical suction request | fault state | notes |
| --- | ---: | --- | ---: | ---: | ---: | --- | --- |
| S01_ground_straight | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND | 500 | 0 | 0 | - | ok; surfaces=GROUND/UNKNOWN |
| S02_ground_curve_left_right | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND | 500 | 45 | 0 | - | ok; surfaces=GROUND/UNKNOWN |
| S03_ground_to_wall_transition_radius | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK | 500 | 0 | 0 | - | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S04_vertical_wall_track | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK | 500 | 0 | 0 | - | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S05_wall_to_ground_transition | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > TRANSITION_DOWN > GROUND_RECOVERY > FINISHED | 500 | 0 | 0 | - | ok; surfaces=GROUND/TRANSITION_DOWN/TRANSITION_UP/UNKNOWN/WALL |
| S06_cylindrical_or_curved_surface | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK | 500 | 30 | 0 | - | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S07_sensor_stale_on_ground | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_FAULT | 500 | 0 | 0 | GROUND_FAULT | ok; surfaces=GROUND/UNKNOWN |
| S08_sensor_stale_on_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 500 | 0 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S09_transition_timeout | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_FAILSAFE_HOLD | 500 | 0 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN |
| S10_suction_unverified | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK | 500 | 0 | 0 | - | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S11_encoder_dropout | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 500 | 0 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S12_emag_loss | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 500 | 0 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S13_imu_axis_or_pitch_abnormal | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 500 | 0 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=CYLINDER/GROUND/TRANSITION_UP/UNKNOWN/WALL |

Current status separation:

- Code logic pass: measured by these Host-SIL assertions.
- Host-SIL pass: measured by `sim/results/summary.json` and per-scenario CSV files.
- Keil C251 build pass: measured separately by the Keil build log.
- Real bench pass: not performed.
- Real wall pass: not performed.
