# Host-SIL Results

These P0 safety-gate results are Host-SIL software results only. Logical Wall Profile enables only a theoretical host-side suction request and does not verify ESC, fan, adhesion, real drive output, or wall capability.

| scenario | profile | pass | actual state sequence | max speed cmd | max steering cmd | max logical suction request | max hardware suction output | fault state | notes |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | --- | --- |
| S01_ground_straight | guard | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK | 145 | 0 | 0 | 0 | - | ok; surfaces=GROUND/UNKNOWN |
| S02_ground_curve_left_right | guard | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK | 145 | 0 | 0 | 0 | - | ok; surfaces=GROUND/UNKNOWN |
| S03_ground_to_wall_transition_radius | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK | 145 | 0 | 1450 | 0 | - | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S04_vertical_wall_track | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK | 145 | 0 | 1450 | 0 | - | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S05_wall_to_ground_transition | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > TRANSITION_DOWN > GROUND_RECOVERY > GROUND_TRACK | 145 | 0 | 1450 | 0 | - | ok; surfaces=GROUND/TRANSITION_DOWN/TRANSITION_UP/UNKNOWN/WALL |
| S06_cylindrical_or_curved_surface | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK | 145 | 0 | 1450 | 0 | - | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S07_sensor_stale_on_ground | guard | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > GROUND_FAULT | 145 | 0 | 0 | 0 | GROUND_FAULT | ok; surfaces=GROUND/UNKNOWN |
| S08_sensor_stale_on_wall | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 145 | 0 | 1450 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S09_transition_timeout | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > WALL_FAILSAFE_HOLD | 145 | 0 | 1450 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN |
| S10_suction_unverified | guard | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > SUCTION_LOCKOUT | 145 | 0 | 0 | 0 | SUCTION_LOCKOUT | ok; surfaces=GROUND/UNKNOWN |
| S11_encoder_dropout | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 145 | 0 | 1450 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S12_emag_loss | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 145 | 0 | 1450 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=GROUND/TRANSITION_UP/UNKNOWN/WALL |
| S13_imu_axis_or_pitch_abnormal | logical_wall | yes | BOOT > SELF_CHECK > SENSOR_CALIBRATION > SAFE_GROUND_READY > ARMED_GROUND > GROUND_TRACK > TRANSITION_CANDIDATE > SUCTION_PRECHARGE > APPROACH_TRANSITION > TRANSITION_UP > WALL_TRACK > WALL_FAILSAFE_HOLD | 145 | 0 | 1450 | 0 | WALL_FAILSAFE_HOLD | ok; surfaces=CYLINDER/GROUND/TRANSITION_DOWN/TRANSITION_UP/UNKNOWN/WALL |

Status separation:

- Guard Profile validates suction lockout while `SUCTION_HW_VERIFIED=0`.
- Logical Wall Profile validates only host-side state-machine flow and logical suction requests.
- Keil C251 build pass is measured separately by the Keil build log.
- Real bench pass: not performed.
- Real wall pass: not performed.
