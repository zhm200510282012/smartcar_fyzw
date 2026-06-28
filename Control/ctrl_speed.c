#include "ctrl_speed.h"

s16 ctrl_speed_command_native(s16 target_mm_s, encoder_sample_t encoder)
{
    s16 command;
    if (encoder.valid == APP_FALSE) {
        return DRIVE_SAFE_ZERO;
    }
    command = target_mm_s;
    if (command > DRIVE_LIMIT_ABS) command = DRIVE_LIMIT_ABS;
    if (command < -DRIVE_LIMIT_ABS) command = -DRIVE_LIMIT_ABS;
    return command;
}
