#ifndef CTRL_SPEED_H
#define CTRL_SPEED_H

#include "../App/app_types.h"
#include "../App/app_config.h"

s16 ctrl_speed_command_native(s16 target_mm_s, encoder_sample_t encoder);

#endif
