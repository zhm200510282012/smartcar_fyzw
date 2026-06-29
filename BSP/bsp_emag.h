#ifndef BSP_EMAG_H
#define BSP_EMAG_H

#include "../App/app_types.h"

void bsp_emag_init(void);
emag_sample_t bsp_emag_read(void);
emag_sample_t bsp_emag_last_sample(void);

#ifdef HOST_SIL
void bsp_emag_host_set_raw(u16 a, u16 b, u16 c, u16 d, u16 e, app_bool_t adc_valid);
#endif

#endif
