# State Machine Transitions

## Normal Ground Flow

| From | Predicate | To |
| --- | --- | --- |
| `BOOT` | scheduler first step | `SELF_CHECK` |
| `SELF_CHECK` | `health.power_ok` | `SENSOR_CALIBRATION` |
| `SENSOR_CALIBRATION` | `sensors_ready && ground_observed` | `SAFE_GROUND_READY` |
| `SAFE_GROUND_READY` | `manual_arm && sensors_ready` | `ARMED_GROUND` |

## Wall Candidate Flow

| From | Predicate | To |
| --- | --- | --- |
| `ARMED_GROUND` | `manual_suction_authorize && sensors_ready && transition_candidate_detected && APP_WALL_STATE_CAPABLE != 0` | `SUCTION_PRECHARGE` |
| `ARMED_GROUND` | `manual_suction_authorize && sensors_ready && transition_candidate_detected && APP_WALL_STATE_CAPABLE == 0` | `SUCTION_LOCKOUT` |
| `SUCTION_PRECHARGE` | candidate remains true, sensors ready, `state_elapsed_ms >= PRECHARGE_MIN_TIME_MS` | `APPROACH_TRANSITION` |
| `APPROACH_TRANSITION` | `transition_up_observed` | `TRANSITION_UP` |
| `TRANSITION_UP` | `wall_observed` | `WALL_TRACK` |
| `WALL_TRACK` | `transition_down_observed` | `TRANSITION_DOWN` |
| `TRANSITION_DOWN` | `ground_observed` | `GROUND_RECOVERY` |
| `GROUND_RECOVERY` | ground confirmed for `GROUND_CONFIRM_TIME_MS` | `FINISHED` |

## Safety Exits

| Active state | Condition | Final handling |
| --- | --- | --- |
| `SUCTION_LOCKOUT` | wall not allowed by profile | final safety forces drive zero, steering center, suction request zero |
| Ground states | stale sensor or line loss | `GROUND_FAULT` |
| Wall-related states | stale IMU, encoder dropout, EMAG loss, timeout, abnormal pitch | `WALL_FAILSAFE_HOLD` |
| Any state | hard power fault | `HARD_FAULT` |

Wall-related states are:

- `SUCTION_PRECHARGE`
- `APPROACH_TRANSITION`
- `TRANSITION_UP`
- `WALL_TRACK`
- `TRANSITION_DOWN`
- `WALL_FAILSAFE_HOLD`

`SUCTION_LOCKOUT` is not a wall-related state. It is a safety refusal before wall-state entry.
