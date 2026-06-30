#ifndef BSP_EMAG_H
#define BSP_EMAG_H

#include "../App/app_types.h"

#define EMAG_FRAME_BUFFER_COUNT 3u
#define EMAG_FRAME_INVALID_INDEX 0xFFu

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
void bsp_emag_host_begin_read_for_test(void);
app_bool_t bsp_emag_host_copy_locked_frame_for_test(emag_frame_t *frame);
void bsp_emag_host_end_read_for_test(void);
void bsp_emag_host_debug_indices(u8 *front, u8 *write, u8 *reader);
#endif

#endif
