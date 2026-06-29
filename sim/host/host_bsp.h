#ifndef HOST_BSP_H
#define HOST_BSP_H

#include "../../App/app_types.h"

typedef struct {
    u32 time_ms;
    u8 manual_arm;
    u8 suction_authorize;
    u8 transition_candidate;
    u8 emag_valid;
    s16 line_error;
    u16 signal_quality;
    u8 emag_norm_valid;
    u16 emag_norm[5];
    u8 imu_fresh;
    u8 imu_id_ok;
    s16 pitch_cdeg;
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
u8 host_bsp_kill_switch(void);
u8 host_bsp_control_period_ok(void);
s16 host_bsp_force_app_state(void);

#endif
