#ifndef APP_CONTROL_TICK_H
#define APP_CONTROL_TICK_H

#include "app_types.h"

typedef struct {
    u16 sensor_frame_sequence;
    u16 control_tick_count;
    u16 speed_pi_pair_calls;
    u16 sensor_frame_stale_count;
    u16 control_no_new_frame_count;
    u16 control_skipped_duplicate_frame_count;
    u16 control_deadline_miss_count;
    u8 last_sensor_frame_complete;
    u8 last_control_used_adc;
} app_control_tick_stats_t;

void app_control_tick_init(void);
void app_control_tick_bind_context(app_context_t *ctx);
app_context_t *app_control_tick_bound_context(void);
void app_control_tick_sensor_isr(app_context_t *ctx, u32 now_ms);
void app_control_tick_control_isr(app_context_t *ctx, u32 now_ms);
app_control_tick_stats_t app_control_tick_stats(void);

#endif
