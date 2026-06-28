#ifndef HOST_BSP_H
#define HOST_BSP_H

#include "../../App/app_types.h"

typedef struct {
    u32 time_ms;
    u8 manual_arm;
    u8 suction_authorize;
    u8 emag_valid;
    s16 line_error;
    u16 signal_quality;
    u8 imu_fresh;
    s16 pitch_cdeg;
    u8 encoder_valid;
    s32 left_count;
    s32 right_count;
    s16 left_speed_mm_s;
    s16 right_speed_mm_s;
    u8 power_ok;
} host_sil_input_t;

void host_bsp_reset(void);
void host_bsp_set_input(const host_sil_input_t *input);

#endif
