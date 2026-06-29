#include "bsp_imu.h"

void bsp_imu_init(void)
{
}

attitude_sample_t bsp_imu_read(void)
{
    attitude_sample_t a;
    a.roll_cdeg = 0;
    a.pitch_cdeg = 0;
    a.pitch_rate_cdeg_s = 0;
    a.yaw_rate_cdeg_s = 0;
    a.timestamp_ms = 0ul;
    a.id_ok = APP_FALSE;
    a.fresh = APP_FALSE;
    return a;
}
