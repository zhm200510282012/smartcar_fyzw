# Host-SIL Test Matrix

| Scenario | Input trajectory | Expected result | Assertions |
| --- | --- | --- | --- |
| S01_ground_straight | flat pitch, valid sensors, straight line | ground_ok | state sequence, fault state, drive/steering range, suction output lock |
| S02_ground_curve_left_right | flat pitch, alternating left/right line error | ground_ok | state sequence, fault state, drive/steering range, suction output lock |
| S03_ground_to_wall_transition_radius | ground-to-wall pitch ramp and valid wall hold | wall_track | state sequence, fault state, drive/steering range, suction output lock |
| S04_vertical_wall_track | ground-to-wall ramp followed by vertical hold | wall_track | state sequence, fault state, drive/steering range, suction output lock |
| S05_wall_to_ground_transition | ground-to-wall ramp, wall hold, transition-down pitch ramp | finished | state sequence, fault state, drive/steering range, suction output lock |
| S06_cylindrical_or_curved_surface | wall pitch with alternating line error | wall_track | state sequence, fault state, drive/steering range, suction output lock |
| S07_sensor_stale_on_ground | ground run with IMU freshness dropped after arming | ground_fault | state sequence, fault state, drive/steering range, suction output lock |
| S08_sensor_stale_on_wall | wall run with IMU freshness dropped during wall track | wall_failsafe | state sequence, fault state, drive/steering range, suction output lock |
| S09_transition_timeout | transition-up pitch without reaching wall threshold | wall_failsafe | state sequence, fault state, drive/steering range, suction output lock |
| S10_suction_unverified | ground-to-wall pitch ramp and valid wall hold | suction_unverified | state sequence, fault state, drive/steering range, suction output lock |
| S11_encoder_dropout | wall run with encoder validity dropped | wall_failsafe | state sequence, fault state, drive/steering range, suction output lock |
| S12_emag_loss | wall run with electromagnetic signal dropped | wall_failsafe | state sequence, fault state, drive/steering range, suction output lock |
| S13_imu_axis_or_pitch_abnormal | wall run with abnormal negative pitch jump | wall_failsafe | state sequence, fault state, drive/steering range, suction output lock |
