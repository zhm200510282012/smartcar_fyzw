#include "bsp_emag.h"
#include "board_emag_map.h"

#ifndef HOST_SIL
#include "AI8051U_ADC.h"
#endif

static emag_sample_t g_last_sample;
static emag_frame_t g_front_frame;
static emag_frame_t g_back_frame;
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

static u16 read_adc_logical_channel(u8 index)
{
#ifdef HOST_SIL
    return g_host_raw[index];
#else
    return Get_ADCResult((ADC_CHx)g_adc_channels[index]);
#endif
}

static void commit_back_frame(u32 now_ms)
{
    g_back_frame.timestamp_ms = now_ms;
    g_back_frame.sequence = (u16)(g_front_frame.sequence + 1u);
    g_back_frame.complete = APP_TRUE;
    g_front_frame = g_back_frame;
}

void bsp_emag_init(void)
{
    u8 i;

    clear_sample(&g_last_sample);
    clear_frame(&g_front_frame);
    clear_frame(&g_back_frame);
    g_sensor_channel_index = 0u;
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

    raw = read_adc_logical_channel(g_sensor_channel_index);
    g_back_frame.raw[g_sensor_channel_index] = raw;
    g_sensor_channel_index++;
    if (g_sensor_channel_index >= EMAG_CHANNEL_COUNT) {
        g_sensor_channel_index = 0u;
        commit_back_frame(now_ms);
    }
}

app_bool_t bsp_emag_latest_frame(emag_frame_t *frame)
{
    if (g_front_frame.complete == APP_FALSE) {
        return APP_FALSE;
    }
    if (frame != 0) {
        *frame = g_front_frame;
    }
    return APP_TRUE;
}

emag_sample_t bsp_emag_sample_from_frame(const emag_frame_t *frame)
{
    emag_sample_t s;
    u8 i;
    app_bool_t all_valid;
    u16 raw;

    clear_sample(&s);
    if (frame == 0 || frame->complete == APP_FALSE) {
        g_last_sample = s;
        return s;
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
    g_last_sample = s;
    return s;
}

emag_sample_t bsp_emag_read(void)
{
    emag_frame_t frame;

#ifdef HOST_SIL
    u8 i;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        g_back_frame.raw[i] = g_host_raw[i];
    }
    commit_back_frame(0ul);
#else
    u8 i;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        g_back_frame.raw[i] = read_adc_logical_channel(i);
    }
    commit_back_frame(0ul);
#endif
    if (bsp_emag_latest_frame(&frame) == APP_FALSE) {
        clear_frame(&frame);
    }
    return bsp_emag_sample_from_frame(&frame);
}

emag_sample_t bsp_emag_last_sample(void)
{
    return g_last_sample;
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
#endif
