#include "bsp_power.h"
#include "../App/app_config.h"
#include "../App/app_memory.h"

#ifndef HOST_SIL
#include "AI8051U_ADC.h"
#endif

static power_sample_t APP_XDATA g_power_sample;
static app_bool_t g_filter_ready;

#ifdef HOST_SIL
static u16 g_host_raw_adc;
static app_bool_t g_host_adc_valid;
#endif

static u16 filtered_adc_update(u16 raw_adc)
{
    u32 accum;

    if (g_filter_ready == APP_FALSE) {
        g_filter_ready = APP_TRUE;
        return raw_adc;
    }

    accum = ((u32)g_power_sample.filtered_adc * (u32)POWER_ADC_FILTER_ALPHA) +
            ((u32)raw_adc * (u32)(POWER_ADC_FILTER_DENOM - POWER_ADC_FILTER_ALPHA));
    return (u16)(accum / (u32)POWER_ADC_FILTER_DENOM);
}

static u16 estimate_voltage_mv(u16 filtered_adc, app_bool_t calibration_valid)
{
    u32 mv;

    if (calibration_valid == APP_FALSE || POWER_ADC_MV_PER_COUNT_DEN == 0ul) {
        return 0u;
    }
    mv = ((u32)filtered_adc * (u32)POWER_ADC_MV_PER_COUNT_NUM) /
         (u32)POWER_ADC_MV_PER_COUNT_DEN;
    if (mv > 65535ul) {
        mv = 65535ul;
    }
    return (u16)mv;
}

void bsp_power_init(void)
{
    g_filter_ready = APP_FALSE;
    g_power_sample.raw_adc = 0u;
    g_power_sample.filtered_adc = 0u;
    g_power_sample.voltage_estimate_mv = 0u;
    g_power_sample.calibration_valid = APP_FALSE;
    g_power_sample.power_good = (POWER_GUARD_ENABLE == 0) ? APP_TRUE : APP_FALSE;
    g_power_sample.power_uncalibrated = APP_TRUE;
    g_power_sample.adc_valid = APP_FALSE;
#ifdef HOST_SIL
    g_host_raw_adc = 0u;
    g_host_adc_valid = APP_TRUE;
#else
    {
        ADC_InitTypeDef adc;
        ADC_GPIO_Init((ADC_CHx)POWER_ADC_CHANNEL);
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

power_sample_t bsp_power_read_sample(void)
{
    u16 raw_adc;
    app_bool_t adc_valid;
    app_bool_t calibration_valid;

#ifdef HOST_SIL
    raw_adc = g_host_raw_adc;
    adc_valid = g_host_adc_valid;
#else
    raw_adc = Get_ADCResult((ADC_CHx)POWER_ADC_CHANNEL);
    adc_valid = (raw_adc < 4096u) ? APP_TRUE : APP_FALSE;
#endif

    if (adc_valid == APP_FALSE) {
        raw_adc = 0u;
    }
    calibration_valid = (POWER_VOLTAGE_CALIBRATION_VALID != 0) ? APP_TRUE : APP_FALSE;

    g_power_sample.raw_adc = raw_adc;
    g_power_sample.filtered_adc = filtered_adc_update(raw_adc);
    g_power_sample.calibration_valid = calibration_valid;
    g_power_sample.power_uncalibrated = (calibration_valid == APP_FALSE) ? APP_TRUE : APP_FALSE;
    g_power_sample.voltage_estimate_mv = estimate_voltage_mv(g_power_sample.filtered_adc,
                                                             calibration_valid);
    g_power_sample.adc_valid = adc_valid;

    if (POWER_GUARD_ENABLE == 0) {
        g_power_sample.power_good = APP_TRUE;
    } else if (adc_valid == APP_FALSE) {
        g_power_sample.power_good = APP_FALSE;
    } else {
        g_power_sample.power_good = (g_power_sample.filtered_adc >= POWER_LOW_RAW_THRESHOLD) ? APP_TRUE : APP_FALSE;
    }

    return g_power_sample;
}

power_sample_t bsp_power_last_sample(void)
{
    return g_power_sample;
}

app_bool_t bsp_power_is_ok(void)
{
    power_sample_t sample;
    sample = bsp_power_read_sample();
    return sample.power_good;
}

#ifdef HOST_SIL
void bsp_power_host_set_raw(u16 raw_adc, app_bool_t adc_valid)
{
    g_host_raw_adc = raw_adc;
    g_host_adc_valid = adc_valid;
}
#endif
