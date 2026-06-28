#ifndef APP_TELEMETRY_H
#define APP_TELEMETRY_H

#include "app_types.h"

typedef struct {
    u32 timestamp_ms;
    app_state_t app_state;
    surface_state_t surface_state;
    fault_code_t faults;
    s16 line_error;
    u16 signal_quality;
    s16 drive_cmd;
    s16 steering_cmd;
    suction_mode_t suction_mode;
    u16 suction_request_native;
    u16 adhesion_risk;
} telemetry_frame_t;

telemetry_frame_t app_telemetry_make_frame(const app_context_t *ctx, u32 now_ms);

#endif
