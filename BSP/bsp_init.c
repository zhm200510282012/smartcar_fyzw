#include "bsp_init.h"
#include "bsp_timebase.h"
#include "bsp_debug_uart.h"
#include "bsp_ui.h"
#include "bsp_drive.h"
#include "bsp_fan_esc.h"
#include "bsp_encoder.h"
#include "bsp_emag.h"
#include "bsp_imu.h"
#include "bsp_power.h"

void bsp_init_all_safe(void)
{
    bsp_timebase_init();
    bsp_debug_uart_init();
    bsp_ui_init();
    bsp_drive_init();
    bsp_fan_esc_init();
    bsp_encoder_init();
    bsp_emag_init();
    bsp_imu_init();
    bsp_power_init();
}
