#include "ctrl_line.h"

emag_sample_t ctrl_line_update(emag_sample_t input)
{
    u16 left;
    u16 right;
    if (input.channel_count < 2u || input.valid == APP_FALSE) {
        input.signal_quality = 0u;
        input.line_error = 0;
        return input;
    }
    left = input.norm[0];
    right = input.norm[input.channel_count - 1u];
    input.line_error = (s16)right - (s16)left;
    input.signal_quality = (u16)(left + right);
    return input;
}
