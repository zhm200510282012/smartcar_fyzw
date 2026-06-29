#ifndef COMPETITION_PROFILE_H
#define COMPETITION_PROFILE_H

/* User-facing competition tuning profile. Keep bench defaults conservative. */

#define LINE_DIRECTION_SIGN 1
#define LINE_WEIGHT_SCALE 1
#define LINE_VALID_SUM_MIN 160u
#define LINE_LOST_QUALITY_MIN 200u
#define LINE_FILTER_ALPHA 3
#define LINE_FILTER_DENOM 4

/* Historical servo settings are retained for old reference files only. */
#define STEERING_LEFT_CENTER_US 1510u
#define STEERING_RIGHT_CENTER_US 1510u
#define STEERING_LEFT_MIN_US 1000u
#define STEERING_LEFT_MAX_US 2000u
#define STEERING_RIGHT_MIN_US 1000u
#define STEERING_RIGHT_MAX_US 2000u
#define STEERING_LEFT_SIGN 1
#define STEERING_RIGHT_SIGN -1

#define SPEED_KP 32
#define SPEED_KI 4
#define SPEED_INTEGRAL_LIMIT 3000l
#define SPEED_OUTPUT_LIMIT 260
#define SPEED_ACCEL_LIMIT 12

#define GROUND_STEERING_KP 115
#define GROUND_STEERING_KD 70
#define FUZZY_ENABLE 0

#define DIFF_TURN_SIGN 1
#define DIFF_TURN_DELTA_LIMIT_MM_S 90
#define DIFF_TARGET_SPEED_LIMIT_MM_S 260
#define DIFF_LINE_LOST_SEARCH_SPEED_MM_S 45
#define DIFF_LINE_LOST_STOP_TIME_MS 250u

#define GROUND_STRAIGHT_SPEED_MM_S 180
#define GROUND_CURVE_SPEED_MM_S 140
#define SHARP_CURVE_SPEED_MM_S 95
#define TRANSITION_SPEED_MM_S 70
#define WALL_SPEED_MM_S 80

/* Fan ESC defaults are software starting points, not real-car calibration. */
#define FAN_PWM_FREQ_HZ 50u
#define FAN_ESC_MIN_US 1000u
#define FAN_ESC_MAX_US 2000u
#define FAN_ESC_ARM_TIME_MS 2500u
#define FAN_PRECHARGE_US 1350u
#define FAN_PRECHARGE_TIME_MS 500u
#define FAN_HOLD_US 1450u
#define FAN_BOOST_US 1600u
#define FAN_BOOST_TIME_MS 300u
#define FAN_RAMP_DOWN_STEP_US 10u
#define FAN_RAMP_DOWN_PERIOD_MS 20u
#define FAN_ESC_PHYSICAL_OUTPUT_ENABLE 0
#define WALL_RUN_ENABLE 0

#define IMU_PITCH_SIGN 1
#define IMU_PITCH_OFFSET_CDEG 0
#define IMU_WALL_ENTER_CDEG 4500
#define IMU_WALL_EXIT_CDEG 2500
#define IMU_TRANSITION_CONFIRM_MS 150u
#define IMU_GROUND_CONFIRM_MS 250u
#define IMU_STALE_TIMEOUT_MS 100u

#if (LINE_DIRECTION_SIGN != 1) && (LINE_DIRECTION_SIGN != -1)
#error LINE_DIRECTION_SIGN must be +1 or -1.
#endif

#if (DIFF_TURN_SIGN != 1) && (DIFF_TURN_SIGN != -1)
#error DIFF_TURN_SIGN must be +1 or -1.
#endif

#if (STEERING_LEFT_SIGN != 1) && (STEERING_LEFT_SIGN != -1)
#error STEERING_LEFT_SIGN must be +1 or -1.
#endif

#if (STEERING_RIGHT_SIGN != 1) && (STEERING_RIGHT_SIGN != -1)
#error STEERING_RIGHT_SIGN must be +1 or -1.
#endif

#if (FUZZY_ENABLE != 0) && (FUZZY_ENABLE != 1)
#error FUZZY_ENABLE must be 0 or 1.
#endif

#if (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0) && (FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 1)
#error FAN_ESC_PHYSICAL_OUTPUT_ENABLE must be 0 or 1.
#endif

#if (WALL_RUN_ENABLE != 0) && (WALL_RUN_ENABLE != 1)
#error WALL_RUN_ENABLE must be 0 or 1.
#endif

#if (IMU_PITCH_SIGN != 1) && (IMU_PITCH_SIGN != -1)
#error IMU_PITCH_SIGN must be +1 or -1.
#endif

#endif
