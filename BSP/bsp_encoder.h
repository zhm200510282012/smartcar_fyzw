#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include "../App/app_types.h"

void bsp_encoder_init(void);
encoder_sample_t bsp_encoder_read(void);

#endif
