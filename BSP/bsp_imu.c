#include "bsp_imu.h"
#include "../App/app_config.h"

static attitude_sample_t g_last_attitude;

static s16 configured_pitch_cdeg(const lsm6dsr_sample_t *sample)
{
    s32 pitch;
    u8 axis;

    axis = IMU_PITCH_AXIS;
    pitch = ((s32)sample->accel_raw[axis] * 10000l) / 16384l;
    pitch = ((s32)IMU_PITCH_SIGN * pitch) + (s32)IMU_PITCH_OFFSET_CDEG;
    if (pitch > 32767l) {
        pitch = 32767l;
    }
    if (pitch < -32768l) {
        pitch = -32768l;
    }
    return (s16)pitch;
}

static void clear_attitude(attitude_sample_t *sample)
{
    sample->roll_cdeg = 0;
    sample->pitch_cdeg = 0;
    sample->pitch_rate_cdeg_s = 0;
    sample->yaw_rate_cdeg_s = 0;
    sample->accel_raw[0] = 0;
    sample->accel_raw[1] = 0;
    sample->accel_raw[2] = 0;
    sample->gyro_raw[0] = 0;
    sample->gyro_raw[1] = 0;
    sample->gyro_raw[2] = 0;
    sample->timestamp_ms = 0ul;
    sample->spi_ok = APP_FALSE;
    sample->who_am_i = 0u;
    sample->id_ok = APP_FALSE;
    sample->fresh = APP_FALSE;
}

void bsp_imu_init(void)
{
    clear_attitude(&g_last_attitude);
    lsm6dsr_driver_init();
}

attitude_sample_t bsp_imu_read(void)
{
    lsm6dsr_sample_t raw;
    u8 i;

    raw = lsm6dsr_driver_read();
    clear_attitude(&g_last_attitude);
    g_last_attitude.spi_ok = raw.spi_ok;
    g_last_attitude.who_am_i = raw.who_am_i;
    g_last_attitude.id_ok = raw.id_ok;
    g_last_attitude.timestamp_ms = raw.timestamp_ms;
    for (i = 0u; i < 3u; i++) {
        g_last_attitude.accel_raw[i] = raw.accel_raw[i];
        g_last_attitude.gyro_raw[i] = raw.gyro_raw[i];
    }
    if (raw.sample_valid != APP_FALSE) {
        g_last_attitude.pitch_cdeg = configured_pitch_cdeg(&raw);
        g_last_attitude.pitch_rate_cdeg_s = raw.gyro_raw[1];
        g_last_attitude.yaw_rate_cdeg_s = raw.gyro_raw[2];
        g_last_attitude.fresh = APP_TRUE;
    }
    return g_last_attitude;
}

attitude_sample_t bsp_imu_last_sample(void)
{
    return g_last_attitude;
}

#ifdef HOST_SIL
void bsp_imu_host_set_sample(app_bool_t spi_ok,
                             u8 who_am_i,
                             s16 ax,
                             s16 ay,
                             s16 az,
                             s16 gx,
                             s16 gy,
                             s16 gz)
{
    lsm6dsr_driver_host_set_sample(spi_ok, who_am_i, ax, ay, az, gx, gy, gz);
}
#endif
