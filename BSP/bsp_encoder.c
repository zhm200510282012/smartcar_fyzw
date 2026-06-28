#include "bsp_encoder.h"

void bsp_encoder_init(void)
{
}

encoder_sample_t bsp_encoder_read(void)
{
    encoder_sample_t e;
    e.left_count = 0l;
    e.right_count = 0l;
    e.left_speed_mm_s = 0;
    e.right_speed_mm_s = 0;
    e.valid = APP_FALSE;
    return e;
}
