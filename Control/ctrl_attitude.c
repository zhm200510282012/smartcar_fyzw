#include "ctrl_attitude.h"

static volatile u16 g_last_attitude_dt_ms;

attitude_sample_t ctrl_attitude_update(attitude_sample_t input, u16 dt_ms)
{
    g_last_attitude_dt_ms = dt_ms;
    return input;
}
