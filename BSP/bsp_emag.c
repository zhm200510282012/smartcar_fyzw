#include "bsp_emag.h"
#include "board_emag_map.h"

#ifndef HOST_SIL
#include "AI8051U_ADC.h"
#endif

static emag_sample_t g_last_sample;

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

void bsp_emag_init(void)
{
    u8 i;

    clear_sample(&g_last_sample);
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
        (void)ADC_Inilize(&adc);
        ADC_PowerControl(1u);
    }
#endif
}

emag_sample_t bsp_emag_read(void)
{
    emag_sample_t s;
    u8 i;
    app_bool_t all_valid;
    u16 raw;

    clear_sample(&s);
    all_valid = APP_TRUE;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
#ifdef HOST_SIL
        raw = g_host_raw[i];
        if (g_host_adc_valid == APP_FALSE) {
            all_valid = APP_FALSE;
        }
#else
        raw = Get_ADCResult((ADC_CHx)g_adc_channels[i]);
        if (raw >= 4096u) {
            raw = 0u;
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
