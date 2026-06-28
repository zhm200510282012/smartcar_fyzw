#include "ctrl_steering.h"

s16 ctrl_steering_command(s16 line_error, u16 signal_quality)
{
    if (signal_quality == 0u) {
        return 0;
    }
    return (s16)(line_error / 4);
}
