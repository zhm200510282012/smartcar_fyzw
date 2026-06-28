#ifndef BSP_EMAG_H
#define BSP_EMAG_H

#include "../App/app_types.h"

void bsp_emag_init(void);
emag_sample_t bsp_emag_read(void);

#endif
