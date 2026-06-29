#ifndef BSP_IMU_H
#define BSP_IMU_H

#include "../App/app_types.h"
#include "../Drivers/Board/lsm6dsr_driver.h"

void bsp_imu_init(void);
attitude_sample_t bsp_imu_read(void);
attitude_sample_t bsp_imu_last_sample(void);

#ifdef HOST_SIL
void bsp_imu_host_set_sample(app_bool_t spi_ok,
                             u8 who_am_i,
                             s16 ax,
                             s16 ay,
                             s16 az,
                             s16 gx,
                             s16 gy,
                             s16 gz);
#endif

#endif
