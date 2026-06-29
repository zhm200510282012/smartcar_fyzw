#ifndef LSM6DSR_DRIVER_H
#define LSM6DSR_DRIVER_H

#include "../../App/app_types.h"

#define LSM6DSR_WHO_AM_I_EXPECTED 0x6Bu

typedef struct {
    app_bool_t spi_ok;
    app_bool_t id_ok;
    app_bool_t sample_valid;
    u8 who_am_i;
    s16 accel_raw[3];
    s16 gyro_raw[3];
    u32 timestamp_ms;
} lsm6dsr_sample_t;

void lsm6dsr_driver_init(void);
lsm6dsr_sample_t lsm6dsr_driver_read(void);
lsm6dsr_sample_t lsm6dsr_driver_last_sample(void);

#ifdef HOST_SIL
void lsm6dsr_driver_host_set_sample(app_bool_t spi_ok,
                                    u8 who_am_i,
                                    s16 ax,
                                    s16 ay,
                                    s16 az,
                                    s16 gx,
                                    s16 gy,
                                    s16 gz);
#endif

#endif
