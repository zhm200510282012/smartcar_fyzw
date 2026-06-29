#ifndef HOST_BSP_H
#define HOST_BSP_H

#include "../../App/app_types.h"
#include "../../Track/track_route_event.h"

typedef struct {
    u32 time_ms;
    u8 manual_arm;
    u8 suction_authorize;
    u8 transition_candidate;
    track_route_event_t route_event;
    u8 emag_valid;
    s16 line_error;
    u16 signal_quality;
    u8 emag_norm_valid;
    u16 emag_norm[5];
    u8 imu_fresh;
    u8 imu_id_ok;
    s16 pitch_cdeg;
    s16 pitch_rate_cdeg_s;
    u8 encoder_valid;
    s32 left_count;
    s32 right_count;
    s16 left_speed_mm_s;
    s16 right_speed_mm_s;
    u8 power_ok;
    u8 kill_switch;
    u8 control_period_ok;
    s16 force_app_state;
} host_sil_input_t;

void host_bsp_reset(void);
void host_bsp_set_input(const host_sil_input_t *input);
u8 host_bsp_transition_candidate(void);
track_route_event_t host_bsp_route_event(void);
u8 host_bsp_kill_switch(void);
u8 host_bsp_control_period_ok(void);
s16 host_bsp_force_app_state(void);
u16 host_bsp_steering_apply_count(void);

#endif
