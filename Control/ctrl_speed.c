#include "ctrl_speed.h"

s16 ctrl_speed_command(s16 target_mm_s, encoder_sample_t encoder)
{
    if (encoder.valid == APP_FALSE) {
        return 0;
    }
    return target_mm_s;
}
