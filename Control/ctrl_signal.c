#include "ctrl_signal.h"

s16 ctrl_signal_clamp_s16(s16 value, s16 min_value, s16 max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

u16 ctrl_signal_abs_s16(s16 value)
{
    if (value < 0) return (u16)(-value);
    return (u16)value;
}
