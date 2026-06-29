#include "bsp_ui.h"
#include "../App/app_config.h"
#include "board_map.h"
#include "bsp_timebase.h"

static volatile app_state_t g_last_shown_state;
static app_bool_t g_last_raw_arm;
static app_bool_t g_debounced_arm;
static u32 g_last_change_ms;

#ifdef HOST_SIL
static app_bool_t g_host_arm_requested;
static app_bool_t g_host_suction_authorized;
#endif

void bsp_ui_init(void)
{
    g_last_shown_state = APP_STATE_BOOT;
    g_last_raw_arm = APP_FALSE;
    g_debounced_arm = APP_FALSE;
    g_last_change_ms = bsp_timebase_now_ms();
#ifdef HOST_SIL
    g_host_arm_requested = APP_FALSE;
    g_host_suction_authorized = APP_FALSE;
#endif
}

static app_bool_t read_raw_arm(void)
{
#if APP_ARM_SOURCE == APP_ARM_SOURCE_ALWAYS_FOR_BENCH
    return APP_TRUE;
#elif APP_ARM_SOURCE == APP_ARM_SOURCE_GO_KEY
#ifdef HOST_SIL
    return g_host_arm_requested;
#else
    if (BOARD_UI_VERIFIED == 0) {
        return APP_FALSE;
    }
    return APP_FALSE;
#endif
#elif APP_ARM_SOURCE == APP_ARM_SOURCE_UART
#ifdef HOST_SIL
    return g_host_arm_requested;
#else
    return APP_FALSE;
#endif
#else
    return APP_FALSE;
#endif
}

app_bool_t bsp_ui_manual_arm_requested(void)
{
    app_bool_t raw;
    u32 now_ms = 0ul;

    raw = read_raw_arm();
#if APP_ARM_SOURCE == APP_ARM_SOURCE_ALWAYS_FOR_BENCH
    (void)now_ms;
    g_last_raw_arm = raw;
    g_debounced_arm = raw;
    return raw;
#else
    now_ms = bsp_timebase_now_ms();
    if (raw != g_last_raw_arm) {
        g_last_raw_arm = raw;
        g_last_change_ms = now_ms;
    }
    if ((now_ms - g_last_change_ms) >= APP_ARM_DEBOUNCE_MS) {
        g_debounced_arm = raw;
    }
    return g_debounced_arm;
#endif
}

app_bool_t bsp_ui_suction_authorized(void)
{
#ifdef HOST_SIL
    return g_host_suction_authorized;
#else
    return APP_FALSE;
#endif
}

void bsp_ui_show_state(app_state_t state)
{
    g_last_shown_state = state;
}

#ifdef HOST_SIL
void bsp_ui_host_set_arm(app_bool_t requested)
{
    g_host_arm_requested = requested;
}

void bsp_ui_host_set_suction_authorized(app_bool_t authorized)
{
    g_host_suction_authorized = authorized;
}
#endif
