#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include "../App/app_types.h"

void bsp_encoder_init(void);
encoder_sample_t bsp_encoder_read(void);
encoder_sample_t bsp_encoder_last_sample(void);

#ifdef HOST_SIL
void bsp_encoder_host_set_delta_counts(s16 left_delta, s16 right_delta, app_bool_t valid);
#endif

#endif
