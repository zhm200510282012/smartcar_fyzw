# Host-SIL Test Matrix

| Scenario | Profile | Input trajectory | Expected result | Assertions |
| --- | --- | --- | --- | --- |
| S01_ground_straight | guard | flat pitch, valid sensors, no transition candidate | ground_ok | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S02_ground_curve_left_right | guard | flat pitch, alternating left/right line error, no transition candidate | ground_ok | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S03_ground_to_wall_transition_radius | logical_wall | candidate event precedes pitch ramp from ground to wall | wall_track | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S04_vertical_wall_track | logical_wall | candidate event, precharge dwell, pitch ramp, vertical hold | wall_track | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S05_wall_to_ground_transition | logical_wall | candidate event, wall hold, transition-down pitch ramp | ground_after_recovery | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S06_cylindrical_or_curved_surface | logical_wall | candidate event, wall pitch with alternating line error | wall_track | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S07_sensor_stale_on_ground | guard | ground run with IMU freshness dropped after arming | ground_fault | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S08_sensor_stale_on_wall | logical_wall | logical wall run with IMU freshness dropped during wall track | wall_failsafe | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S09_transition_timeout | logical_wall | candidate event and transition-up pitch without reaching wall threshold | wall_failsafe | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S10_suction_unverified | guard | guard profile candidate event and suction authorization with no verified suction | suction_lockout | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S11_encoder_dropout | logical_wall | logical wall run with encoder validity dropped | wall_failsafe | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S12_emag_loss | logical_wall | logical wall run with electromagnetic signal dropped | wall_failsafe | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
| S13_imu_axis_or_pitch_abnormal | logical_wall | logical wall run with abnormal negative pitch jump | wall_failsafe | state sequence, profile gates, precharge ordering, command ranges, suction output lock |
