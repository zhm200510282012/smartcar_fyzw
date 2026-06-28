#include "app_params.h"
#include "app_config.h"

static const app_param_entry_t g_params[] = {
    {"SUCTION_HW_VERIFIED", SUCTION_HW_VERIFIED, "bool", "0 or 1", "unverified", "1 enables actuator path after evidence only"},
    {"SUCTION_SAFE_OFF_NATIVE", SUCTION_SAFE_OFF_NATIVE, "native", "ESC protocol dependent", "unverified", "must be proven by oscilloscope"},
    {"SUCTION_PRECHARGE_NATIVE", SUCTION_PRECHARGE_NATIVE, "native", "ESC protocol dependent", "unverified", "nonzero value forbidden before bench test"},
    {"SUCTION_HOLD_NATIVE", SUCTION_HOLD_NATIVE, "native", "ESC protocol dependent", "unverified", "requires static wall evidence"},
    {"SUCTION_BOOST_NATIVE", SUCTION_BOOST_NATIVE, "native", "ESC protocol dependent", "unverified", "thermal and timeout risk"},
    {"SUCTION_EMERGENCY_HOLD_NATIVE", SUCTION_EMERGENCY_HOLD_NATIVE, "native", "ESC protocol dependent", "unverified", "requires safe holding proof"},
    {"TASK_CONTROL_PERIOD_MS", TASK_CONTROL_PERIOD_MS, "ms", "timer dependent", "unverified", "overrun changes stability"},
    {"LINE_QUALITY_MIN", LINE_QUALITY_MIN, "adc_norm", "5LC calibration dependent", "unverified", "wrong threshold causes false line loss"}
};

const app_param_entry_t *app_params_table(u16 *count)
{
    if (count != 0) {
        *count = (u16)(sizeof(g_params) / sizeof(g_params[0]));
    }
    return g_params;
}
