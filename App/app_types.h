#ifndef APP_TYPES_H
#define APP_TYPES_H

#ifdef HOST_SIL
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned int u16;
typedef signed int s16;
typedef unsigned long u32;
typedef signed long s32;
#else
#include "../Drivers/Official_LQ/User/DEF.h"
#endif

typedef u8 app_bool_t;

#define APP_TRUE 1u
#define APP_FALSE 0u
#define EMAG_CHANNEL_COUNT 5u

typedef enum {
    EMAG_A_LEFT = 0,
    EMAG_B_LEFT_MID,
    EMAG_C_CENTER,
    EMAG_D_RIGHT_MID,
    EMAG_E_RIGHT
} emag_channel_t;

typedef enum {
    SURFACE_UNKNOWN = 0,
    SURFACE_GROUND,
    SURFACE_TRANSITION_UP,
    SURFACE_WALL,
    SURFACE_CYLINDER,
    SURFACE_TRANSITION_DOWN,
    SURFACE_DISABLED_UNVERIFIED
} surface_state_t;

typedef enum {
    TRACK_MODE_STRAIGHT = 0,
    TRACK_MODE_NORMAL_CURVE,
    TRACK_MODE_SHARP_CURVE,
    TRACK_MODE_CROSSING,
    TRACK_MODE_OMEGA,
    TRACK_MODE_HEX_LOOP,
    TRACK_MODE_TRANSITION,
    TRACK_MODE_WALL,
    TRACK_MODE_CYLINDER,
    TRACK_MODE_SEESAW,
    TRACK_MODE_LINE_LOST,
    TRACK_MODE_RECOVERY
} track_mode_t;

typedef enum {
    TRACK_COURSE_START = 0,
    TRACK_COURSE_GROUND_STRAIGHT,
    TRACK_COURSE_NORMAL_CURVE,
    TRACK_COURSE_SHARP_CURVE,
    TRACK_COURSE_CROSSING,
    TRACK_COURSE_OMEGA,
    TRACK_COURSE_HEX_LOOP,
    TRACK_COURSE_WALL_APPROACH,
    TRACK_COURSE_FAN_PRECHARGE,
    TRACK_COURSE_TRANSITION_UP,
    TRACK_COURSE_WALL_TRACK,
    TRACK_COURSE_CYLINDER_TRACK,
    TRACK_COURSE_TRANSITION_DOWN,
    TRACK_COURSE_GROUND_RECOVERY,
    TRACK_COURSE_FINISH
} track_course_segment_t;

typedef enum {
    APP_STATE_BOOT = 0,
    APP_STATE_SELF_CHECK,
    APP_STATE_SENSOR_CALIBRATION,
    APP_STATE_SAFE_GROUND_READY,
    APP_STATE_ARMED_GROUND,
    APP_STATE_GROUND_TRACK,
    APP_STATE_TRANSITION_CANDIDATE,
    APP_STATE_SUCTION_PRECHARGE,
    APP_STATE_APPROACH_TRANSITION,
    APP_STATE_TRANSITION_UP,
    APP_STATE_WALL_TRACK,
    APP_STATE_CYLINDER_TRACK,
    APP_STATE_TRANSITION_DOWN,
    APP_STATE_GROUND_RECOVERY,
    APP_STATE_SEESAW_PASS,
    APP_STATE_FINISHED,
    APP_STATE_GROUND_FAULT,
    APP_STATE_SUCTION_LOCKOUT,
    APP_STATE_WALL_FAILSAFE_HOLD,
    APP_STATE_HARD_FAULT
} app_state_t;

typedef enum {
    SUCTION_OFF = 0,
    SUCTION_IDLE,
    SUCTION_PRECHARGE,
    SUCTION_HOLD,
    SUCTION_BOOST,
    SUCTION_COOLDOWN,
    SUCTION_EMERGENCY_HOLD
} suction_mode_t;

typedef enum {
    FAN_ESC_OFF = 0,
    FAN_ESC_ARMING,
    FAN_ESC_PRECHARGE,
    FAN_ESC_HOLD,
    FAN_ESC_BOOST,
    FAN_ESC_RAMP_DOWN,
    FAN_ESC_FAILSAFE_HOLD
} fan_esc_state_t;

typedef enum {
    TRACK_WALL_GROUND_TRACK = 0,
    TRACK_WALL_WALL_APPROACH,
    TRACK_WALL_FAN_PRECHARGE,
    TRACK_WALL_TRANSITION_UP,
    TRACK_WALL_WALL_TRACK,
    TRACK_WALL_CYLINDER_TRACK,
    TRACK_WALL_TRANSITION_DOWN,
    TRACK_WALL_GROUND_RECOVERY,
    TRACK_WALL_FAILSAFE_HOLD
} track_wall_state_t;

typedef enum {
    FAULT_NONE = 0,
    FAULT_SENSOR_STALE = 1u,
    FAULT_LINE_LOST = 2u,
    FAULT_ENCODER_INVALID = 4u,
    FAULT_CONTROL_OVERRUN = 8u,
    FAULT_POWER_RISK = 16u,
    FAULT_SUCTION_UNVERIFIED = 32u,
    FAULT_HARD_POWER = 64u,
    FAULT_SUCTION_LOCKOUT = 128u,
    FAULT_IMU_INVALID = 256u,
    FAULT_KILL_SWITCH = 512u
} fault_code_t;

typedef struct {
    suction_mode_t mode;
    u16 command_native;
    u8 armed;
    u8 hw_verified;
    u8 feedback_valid;
    u8 fault_code;
} suction_command_t;

typedef struct {
    fan_esc_state_t state;
    u16 request_us;
    u16 output_us;
    u8 mapped;
    u8 physical_enabled;
    u8 bench_verified;
} fan_esc_command_t;

typedef struct {
    u16 raw[EMAG_CHANNEL_COUNT];
    u16 filtered[EMAG_CHANNEL_COUNT];
    u16 norm[EMAG_CHANNEL_COUNT];
    s16 line_error;
    u16 line_quality;
    u16 signal_quality;
    u8 channel_count;
    u8 line_lost;
    u8 valid;
} emag_sample_t;

typedef struct {
    s16 roll_cdeg;
    s16 pitch_cdeg;
    s16 pitch_rate_cdeg_s;
    s16 yaw_rate_cdeg_s;
    s16 accel_raw[3];
    s16 gyro_raw[3];
    u32 timestamp_ms;
    u8 spi_ok;
    u8 who_am_i;
    u8 id_ok;
    u8 fresh;
} attitude_sample_t;

typedef struct {
    s32 left_count;
    s32 right_count;
    s16 left_delta_counts;
    s16 right_delta_counts;
    s16 left_speed_counts_per_s;
    s16 right_speed_counts_per_s;
    s16 left_speed_mm_s;
    s16 right_speed_mm_s;
    s32 left_distance_mm;
    s32 right_distance_mm;
    u8 speed_mm_s_valid;
    u8 progress_mm_valid;
    u8 valid;
} encoder_sample_t;

typedef struct {
    u8 emag_ok;
    u8 imu_fresh;
    u8 encoder_ok;
    u8 power_ok;
    u8 control_period_ok;
    u8 suction_feedback_ok;
} sensor_health_t;

typedef struct {
    app_state_t app_state;
    surface_state_t surface_state;
    fault_code_t faults;
    u16 state_elapsed_ms;
    u8 manual_arm;
    u8 manual_suction_authorize;
    u8 transition_candidate;
    u8 kill_switch;
    u8 test_mode;
    track_mode_t track_mode;
    track_course_segment_t course_segment;
    u16 adhesion_risk;
    s16 speed_limit_mm_s;
    s16 target_speed_mm_s;
    s16 left_speed_target_mm_s;
    s16 right_speed_target_mm_s;
    s16 drive_command_native;
    s16 left_drive_command_native;
    s16 right_drive_command_native;
    s16 line_error_filtered;
    s16 line_error_rate;
    s16 fuzzy_kp;
    s16 fuzzy_ki;
    s16 fuzzy_kd;
    s16 turn_delta_mm_s;
    s16 steering_offset_us;
    u16 steering_pulse_us;
    u16 steering_left_pulse_us;
    u16 steering_right_pulse_us;
    suction_command_t suction_cmd;
    fan_esc_command_t fan_cmd;
    track_wall_state_t wall_state;
    emag_sample_t emag;
    attitude_sample_t attitude;
    encoder_sample_t encoder;
    sensor_health_t health;
} app_context_t;

#endif
