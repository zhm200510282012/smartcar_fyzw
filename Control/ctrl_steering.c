/* historical / inactive / not used by runtime: steering offset is not part of the final differential output chain. */
#include "ctrl_steering.h"

s16 ctrl_steering_offset_us(s16 line_error, u16 signal_quality)
{
    s16 offset;
    if (signal_quality == 0u) {
        return 0;
    }
    offset = (s16)(line_error / 4);
    if (offset > STEERING_MAX_OFFSET_US) offset = STEERING_MAX_OFFSET_US;
    if (offset < -STEERING_MAX_OFFSET_US) offset = -STEERING_MAX_OFFSET_US;
    return offset;
}
