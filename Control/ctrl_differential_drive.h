#ifndef CTRL_DIFFERENTIAL_DRIVE_H
#define CTRL_DIFFERENTIAL_DRIVE_H

#include "../App/app_types.h"
#include "../App/app_config.h"

typedef struct {
    s16 left_target_mm_s;
    s16 right_target_mm_s;
    u8 valid;
} differential_drive_output_t;

differential_drive_output_t ctrl_differential_drive_mix(s16 base_speed_mm_s,
                                                        s16 turn_delta_mm_s,
                                                        u8 outputs_allowed);

#endif
