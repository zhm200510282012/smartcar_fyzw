#include "app_hardware_diagnostic.h"
#include <stdio.h>
#include "../BSP/bsp_debug_uart.h"
#include "../BSP/bsp_emag.h"
#include "../BSP/bsp_encoder.h"
#include "../BSP/bsp_imu.h"
#include "../BSP/bsp_ui.h"
#include "../Track/track_route_event.h"

void app_hardware_diagnostic_init(void)
{
    bsp_debug_uart_init();
}

app_hardware_diagnostic_snapshot_t app_hardware_diagnostic_run_once(void)
{
    static char line[240];
    app_hardware_diagnostic_snapshot_t snapshot;

    snapshot.power = bsp_power_read_sample();
    snapshot.arm_requested = bsp_ui_manual_arm_requested();
    bsp_emag_read(&snapshot.emag);
    snapshot.attitude = bsp_imu_read();
    snapshot.encoder = bsp_encoder_read();
    snapshot.route_progress_status = (u8)track_route_event_progress_status();

    sprintf(line,
            "POWER raw=%u filt=%u cal=%u good=%u ARM=%u EMAG=%u,%u,%u,%u,%u IMU id=%u spi=%u ax=%d ay=%d az=%d pitch=%d ENC dl=%d dr=%d route=%u",
            snapshot.power.raw_adc,
            snapshot.power.filtered_adc,
            snapshot.power.calibration_valid,
            snapshot.power.power_good,
            snapshot.arm_requested,
            snapshot.emag.raw[0],
            snapshot.emag.raw[1],
            snapshot.emag.raw[2],
            snapshot.emag.raw[3],
            snapshot.emag.raw[4],
            snapshot.attitude.who_am_i,
            snapshot.attitude.spi_ok,
            snapshot.attitude.accel_raw[0],
            snapshot.attitude.accel_raw[1],
            snapshot.attitude.accel_raw[2],
            snapshot.attitude.pitch_cdeg,
            snapshot.encoder.left_delta_counts,
            snapshot.encoder.right_delta_counts,
            snapshot.route_progress_status);
    bsp_debug_uart_write_line(line);
    return snapshot;
}
