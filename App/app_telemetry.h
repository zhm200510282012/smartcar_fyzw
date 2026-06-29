#ifndef APP_TELEMETRY_H
#define APP_TELEMETRY_H

#include "app_types.h"

typedef struct {
    u32 timestamp_ms;
    app_state_t app_state;
    surface_state_t surface_state;
    track_mode_t track_mode;
    fault_code_t faults;
    s16 line_error;
    s16 line_error_filtered;
    s16 error_rate;
    u16 signal_quality;
    s16 drive_command_native;
    s16 steering_offset_us;
    u16 steering_pulse_us;
    u16 steering_left_pulse_us;
    u16 steering_right_pulse_us;
    s16 fuzzy_kp;
    s16 fuzzy_ki;
    s16 fuzzy_kd;
    s16 left_speed_target_mm_s;
    s16 right_speed_target_mm_s;
    s16 left_speed_measured_mm_s;
    s16 right_speed_measured_mm_s;
    suction_mode_t suction_mode;
    u16 suction_request_native;
    u16 adhesion_risk;
} telemetry_frame_t;

telemetry_frame_t app_telemetry_make_frame(const app_context_t *ctx, u32 now_ms);

#endif
