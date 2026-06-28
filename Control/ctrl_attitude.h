#ifndef CTRL_ATTITUDE_H
#define CTRL_ATTITUDE_H

#include "../App/app_types.h"
#include "../App/app_config.h"

attitude_sample_t ctrl_attitude_update(attitude_sample_t input, u16 dt_ms);

#endif
