#ifndef BSP_IMU_H
#define BSP_IMU_H

#include "../App/app_types.h"

void bsp_imu_init(void);
attitude_sample_t bsp_imu_read(void);

#endif
