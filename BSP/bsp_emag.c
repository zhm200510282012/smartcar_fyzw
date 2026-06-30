#include "bsp_emag.h"
#include "board_emag_map.h"

#ifndef HOST_SIL
#include "AI8051U_ADC.h"
#include "AI8051U_NVIC.h"
#endif

static emag_sample_t g_last_sample;
static emag_frame_t g_frame_buffers[EMAG_FRAME_BUFFER_COUNT];
static volatile u8 g_front_index;
static volatile u8 g_write_index;
static volatile u8 g_reader_index;
static u16 g_next_sequence;
static u8 g_sensor_channel_index;

#ifdef HOST_SIL
static u16 g_host_raw[EMAG_CHANNEL_COUNT];
static app_bool_t g_host_adc_valid;
#endif

#ifndef HOST_SIL
static const u8 g_adc_channels[EMAG_CHANNEL_COUNT] = {
    BOARD_EMAG_A_ADC_CHANNEL,
    BOARD_EMAG_B_ADC_CHANNEL,
    BOARD_EMAG_C_ADC_CHANNEL,
    BOARD_EMAG_D_ADC_CHANNEL,
    BOARD_EMAG_E_ADC_CHANNEL
};
#endif

static const s16 g_offsets[EMAG_CHANNEL_COUNT] = {
    BOARD_EMAG_A_OFFSET,
    BOARD_EMAG_B_OFFSET,
    BOARD_EMAG_C_OFFSET,
    BOARD_EMAG_D_OFFSET,
    BOARD_EMAG_E_OFFSET
};

static const s32 g_gains[EMAG_CHANNEL_COUNT] = {
    BOARD_EMAG_A_GAIN,
    BOARD_EMAG_B_GAIN,
    BOARD_EMAG_C_GAIN,
    BOARD_EMAG_D_GAIN,
    BOARD_EMAG_E_GAIN
};

static const u8 g_invert[EMAG_CHANNEL_COUNT] = {
    BOARD_EMAG_A_INVERT,
    BOARD_EMAG_B_INVERT,
    BOARD_EMAG_C_INVERT,
    BOARD_EMAG_D_INVERT,
    BOARD_EMAG_E_INVERT
};

static u16 clamp_u16_from_s32(s32 value)
{
    if (value < 0l) {
        return 0u;
    }
    if (value > 65535l) {
        return 65535u;
    }
    return (u16)value;
}

static u16 logical_from_raw(u16 raw, u8 index)
{
    s32 adjusted;

    if (g_invert[index] != 0u) {
        adjusted = (s32)4095u - (s32)raw;
    } else {
        adjusted = (s32)raw;
    }
    adjusted = adjusted - (s32)g_offsets[index];
    adjusted = (adjusted * g_gains[index]) / BOARD_EMAG_GAIN_SCALE;
    return clamp_u16_from_s32(adjusted);
}

static void clear_sample(emag_sample_t *sample)
{
    u8 i;

    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        sample->raw[i] = 0u;
        sample->filtered[i] = 0u;
        sample->norm[i] = 0u;
    }
    sample->line_error = 0;
    sample->line_quality = 0u;
    sample->signal_quality = 0u;
    sample->channel_count = EMAG_CHANNEL_COUNT;
    sample->line_lost = APP_TRUE;
    sample->valid = APP_FALSE;
}

static void clear_frame(emag_frame_t *frame)
{
    u8 i;

    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        frame->raw[i] = 0u;
    }
    frame->timestamp_ms = 0ul;
    frame->sequence = 0u;
    frame->complete = APP_FALSE;
}

static void copy_emag_frame(emag_frame_t *dst, const emag_frame_t *src)
{
    u8 i;

    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        dst->raw[i] = src->raw[i];
    }
    dst->timestamp_ms = src->timestamp_ms;
    dst->sequence = src->sequence;
    dst->complete = src->complete;
}

static void clear_write_frame(void)
{
    clear_frame(&g_frame_buffers[g_write_index]);
}

static u16 read_adc_logical_channel(u8 index)
{
#ifdef HOST_SIL
    return g_host_raw[index];
#else
    return Get_ADCResult((ADC_CHx)g_adc_channels[index]);
#endif
}

static void select_next_write_buffer(void)
{
    u8 i;

    for (i = 0u; i < EMAG_FRAME_BUFFER_COUNT; i++) {
        if (i != g_front_index && i != g_reader_index) {
            g_write_index = i;
            clear_write_frame();
            return;
        }
    }
    g_write_index = EMAG_FRAME_INVALID_INDEX;
}

static void publish_write_frame(u32 now_ms)
{
    if (g_write_index == EMAG_FRAME_INVALID_INDEX) {
        return;
    }

    g_frame_buffers[g_write_index].timestamp_ms = now_ms;
    g_frame_buffers[g_write_index].sequence = g_next_sequence;
    g_frame_buffers[g_write_index].complete = APP_TRUE;
    g_next_sequence = (u16)(g_next_sequence + 1u);
    g_front_index = g_write_index;
    select_next_write_buffer();
}

static u8 lock_front_for_read(void)
{
    u8 index;

#ifndef HOST_SIL
    Timer1_Interrupt(DISABLE);
#endif
    index = g_front_index;
    g_reader_index = index;
#ifndef HOST_SIL
    Timer1_Interrupt(ENABLE);
#endif
    return index;
}

void bsp_emag_init(void)
{
    u8 i;

    clear_sample(&g_last_sample);
    for (i = 0u; i < EMAG_FRAME_BUFFER_COUNT; i++) {
        clear_frame(&g_frame_buffers[i]);
    }
    g_front_index = EMAG_FRAME_INVALID_INDEX;
    g_write_index = 0u;
    g_reader_index = EMAG_FRAME_INVALID_INDEX;
    g_next_sequence = 1u;
    g_sensor_channel_index = 0u;
    clear_write_frame();
#ifdef HOST_SIL
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        g_host_raw[i] = 0u;
    }
    g_host_adc_valid = APP_FALSE;
#else
    {
        ADC_InitTypeDef adc;
        for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
            ADC_GPIO_Init((ADC_CHx)g_adc_channels[i]);
        }
        adc.ADC_SMPduty = 20u;
        adc.ADC_Speed = ADC_SPEED_2X16T;
        adc.ADC_AdjResult = ADC_RIGHT_JUSTIFIED;
        adc.ADC_CsSetup = 0u;
        adc.ADC_CsHold = 1u;
        ADC_Inilize(&adc);
        ADC_PowerControl(1u);
    }
#endif
}

void bsp_emag_sensor_tick(u32 now_ms)
{
    u16 raw;

    if (g_write_index == EMAG_FRAME_INVALID_INDEX) {
        select_next_write_buffer();
        if (g_write_index == EMAG_FRAME_INVALID_INDEX) {
            return;
        }
    }
    raw = read_adc_logical_channel(g_sensor_channel_index);
    g_frame_buffers[g_write_index].raw[g_sensor_channel_index] = raw;
    g_sensor_channel_index++;
    if (g_sensor_channel_index >= EMAG_CHANNEL_COUNT) {
        g_sensor_channel_index = 0u;
        publish_write_frame(now_ms);
    }
}

app_bool_t bsp_emag_latest_frame(emag_frame_t *frame)
{
    u8 index;

    if (g_front_index == EMAG_FRAME_INVALID_INDEX) {
        return APP_FALSE;
    }
    index = lock_front_for_read();
    if (index == EMAG_FRAME_INVALID_INDEX ||
        g_frame_buffers[index].complete == APP_FALSE) {
        g_reader_index = EMAG_FRAME_INVALID_INDEX;
        return APP_FALSE;
    }
    if (frame != 0) {
        copy_emag_frame(frame, &g_frame_buffers[index]);
    }
    g_reader_index = EMAG_FRAME_INVALID_INDEX;
    return APP_TRUE;
}

static void copy_emag_sample(emag_sample_t *dst, const emag_sample_t *src)
{
    u8 i;

    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        dst->raw[i] = src->raw[i];
        dst->filtered[i] = src->filtered[i];
        dst->norm[i] = src->norm[i];
    }
    dst->line_error = src->line_error;
    dst->line_quality = src->line_quality;
    dst->signal_quality = src->signal_quality;
    dst->channel_count = src->channel_count;
    dst->line_lost = src->line_lost;
    dst->valid = src->valid;
}

void bsp_emag_sample_from_frame(const emag_frame_t *frame, emag_sample_t *out)
{
    emag_sample_t s;
    u8 i;
    app_bool_t all_valid;
    u16 raw;

    clear_sample(&s);
    if (frame == 0 || frame->complete == APP_FALSE) {
        copy_emag_sample(&g_last_sample, &s);
        if (out != 0) {
            copy_emag_sample(out, &s);
        }
        return;
    }

    all_valid = APP_TRUE;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        raw = frame->raw[i];
#ifndef HOST_SIL
        if (raw >= 4096u) {
            raw = 0u;
            all_valid = APP_FALSE;
        }
#else
        if (g_host_adc_valid == APP_FALSE) {
            all_valid = APP_FALSE;
        }
#endif
        s.raw[i] = raw;
        s.filtered[i] = raw;
        s.norm[i] = logical_from_raw(raw, i);
        s.signal_quality = (u16)(s.signal_quality + s.norm[i]);
    }
    s.line_quality = s.signal_quality;
    s.line_lost = (all_valid != APP_FALSE) ? APP_FALSE : APP_TRUE;
    s.valid = all_valid;
    copy_emag_sample(&g_last_sample, &s);
    if (out != 0) {
        copy_emag_sample(out, &s);
    }
}

void bsp_emag_read(emag_sample_t *out)
{
    emag_frame_t frame;
    u8 i;

#ifdef HOST_SIL
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        g_frame_buffers[g_write_index].raw[i] = g_host_raw[i];
    }
    publish_write_frame(0ul);
#else
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        g_frame_buffers[g_write_index].raw[i] = read_adc_logical_channel(i);
    }
    publish_write_frame(0ul);
#endif
    if (bsp_emag_latest_frame(&frame) == APP_FALSE) {
        clear_frame(&frame);
    }
    bsp_emag_sample_from_frame(&frame, out);
}

void bsp_emag_last_sample(emag_sample_t *out)
{
    if (out != 0) {
        copy_emag_sample(out, &g_last_sample);
    }
}

#ifdef HOST_SIL
void bsp_emag_host_set_raw(u16 a, u16 b, u16 c, u16 d, u16 e, app_bool_t adc_valid)
{
    g_host_raw[0] = a;
    g_host_raw[1] = b;
    g_host_raw[2] = c;
    g_host_raw[3] = d;
    g_host_raw[4] = e;
    g_host_adc_valid = adc_valid;
}

void bsp_emag_host_begin_read_for_test(void)
{
    g_reader_index = g_front_index;
}

app_bool_t bsp_emag_host_copy_locked_frame_for_test(emag_frame_t *frame)
{
    if (g_reader_index == EMAG_FRAME_INVALID_INDEX) {
        return APP_FALSE;
    }
    if (frame != 0) {
        copy_emag_frame(frame, &g_frame_buffers[g_reader_index]);
    }
    return APP_TRUE;
}

void bsp_emag_host_end_read_for_test(void)
{
    g_reader_index = EMAG_FRAME_INVALID_INDEX;
}

void bsp_emag_host_debug_indices(u8 *front, u8 *write, u8 *reader)
{
    if (front != 0) {
        *front = g_front_index;
    }
    if (write != 0) {
        *write = g_write_index;
    }
    if (reader != 0) {
        *reader = g_reader_index;
    }
}
#endif
