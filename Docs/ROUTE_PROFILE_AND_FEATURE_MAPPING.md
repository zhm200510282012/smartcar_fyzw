# Route Profile And Feature Mapping

## Route Evidence Status

No unique, evidence-backed real driving order was found in the current repository inputs for this upgrade. Therefore the firmware uses generic adaptive mode selection by default.

`track_route_profile_configured_count()` currently returns `0`.

No hardcoded route sequence is used. Crossings and complex elements do not imply left, right, or straight decisions unless a future evidence-backed `route_segment_profile[]` is added.

## Two-Layer Mechanism

| Layer | Current status | Behavior |
| --- | --- | --- |
| Generic adaptive layer | enabled | selects mode from observable error, error rate, line quality, surface state, and confirmed features |
| Optional route profile layer | empty | reserved for a future confirmed route table |

## Mode Mapping

| Mode | Observable trigger | Control profile |
| --- | --- | --- |
| `TRACK_MODE_STRAIGHT` | small error, small rate, good quality, ground | low Kp, medium Kd, tiny straight-only Ki, high speed |
| `TRACK_MODE_NORMAL_CURVE` | moderate sustained error/rate | higher Kp, medium Kd, medium speed |
| `TRACK_MODE_SHARP_CURVE` | large error or large rate | high Kp, high Kd, Ki off, low speed |
| `TRACK_MODE_CROSSING` | confirmed crossing feature | conservative profile, no route-choice assumption |
| `TRACK_MODE_OMEGA` | large error with fast rate | high Kp/Kd, low speed |
| `TRACK_MODE_HEX_LOOP` | confirmed hex feature | medium-high Kp, high D, limited steering rate |
| `TRACK_MODE_TRANSITION` | transition surface state | medium Kp, high Kd, Ki off, very low speed |
| `TRACK_MODE_WALL` | wall surface state | medium-high Kp, high Kd, low speed |
| `TRACK_MODE_CYLINDER` | cylinder/curved surface state | high Kp/Kd, low speed |
| `TRACK_MODE_SEESAW` | confirmed seesaw feature | medium Kp, low rate limit, low speed |
| `TRACK_MODE_LINE_LOST` | line quality below threshold | integral reset, steering center, speed zero |
| `TRACK_MODE_RECOVERY` | ground recovery / fault recovery path | low speed, integral reset on entry |

Mode switching uses `TRACK_MODE_DEBOUNCE_COUNT` and `TRACK_MODE_MIN_DWELL_MS`; it is not allowed to switch on every control tick.
