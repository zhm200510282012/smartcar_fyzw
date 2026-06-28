#ifndef CTRL_SIGNAL_H
#define CTRL_SIGNAL_H

#include "../App/app_types.h"
#include "../App/app_config.h"

s16 ctrl_signal_clamp_s16(s16 value, s16 min_value, s16 max_value);
u16 ctrl_signal_abs_s16(s16 value);

#endif
