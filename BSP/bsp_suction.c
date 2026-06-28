#include "bsp_suction.h"
#include "../App/app_config.h"

static suction_command_t g_last_command;
static u16 g_last_native_output;

#if SUCTION_HW_VERIFIED != 0
static u16 clamp_native(u16 value)
{
    if (value > SUCTION_BOOST_NATIVE) {
        return SUCTION_BOOST_NATIVE;
    }
    return value;
}
#endif

static void bsp_suction_write_native(u16 native_value)
{
    /* Native hardware write belongs only here. No register is written until board evidence exists. */
    g_last_native_output = native_value;
}

void bsp_suction_init(void)
{
    g_last_command.mode = SUCTION_OFF;
    g_last_command.command_native = SUCTION_SAFE_OFF_NATIVE;
    g_last_command.armed = APP_FALSE;
    g_last_command.hw_verified = SUCTION_HW_VERIFIED;
    g_last_command.feedback_valid = APP_FALSE;
    g_last_command.fault_code = FAULT_SUCTION_UNVERIFIED;
    bsp_suction_write_native(SUCTION_SAFE_OFF_NATIVE);
}

void bsp_suction_apply(const suction_command_t *cmd)
{
    suction_command_t safe_cmd;

    if (cmd == 0) {
        bsp_suction_init();
        return;
    }

    safe_cmd = *cmd;

#if SUCTION_HW_VERIFIED == 0
    safe_cmd.mode = SUCTION_OFF;
    safe_cmd.command_native = SUCTION_SAFE_OFF_NATIVE;
    safe_cmd.armed = APP_FALSE;
    safe_cmd.hw_verified = APP_FALSE;
    safe_cmd.feedback_valid = APP_FALSE;
    safe_cmd.fault_code = (u8)(safe_cmd.fault_code | FAULT_SUCTION_UNVERIFIED);
    g_last_command = safe_cmd;
    bsp_suction_write_native(SUCTION_SAFE_OFF_NATIVE);
#else
    safe_cmd.command_native = clamp_native(safe_cmd.command_native);
    safe_cmd.hw_verified = APP_TRUE;
    g_last_command = safe_cmd;
    bsp_suction_write_native(safe_cmd.command_native);
#endif
}

suction_command_t bsp_suction_last_command(void)
{
    return g_last_command;
}

u16 bsp_suction_last_native_output(void)
{
    return g_last_native_output;
}
