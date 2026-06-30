#ifndef BSP_EMAG_H
#define BSP_EMAG_H

#include "../App/app_types.h"

typedef struct {
    u16 raw[EMAG_CHANNEL_COUNT];
    u32 timestamp_ms;
    u16 sequence;
    app_bool_t complete;
} emag_frame_t;

void bsp_emag_init(void);
void bsp_emag_sensor_tick(u32 now_ms);
app_bool_t bsp_emag_latest_frame(emag_frame_t *frame);
emag_sample_t bsp_emag_sample_from_frame(const emag_frame_t *frame);
emag_sample_t bsp_emag_read(void);
emag_sample_t bsp_emag_last_sample(void);

#ifdef HOST_SIL
void bsp_emag_host_set_raw(u16 a, u16 b, u16 c, u16 d, u16 e, app_bool_t adc_valid);
#endif

#endif
