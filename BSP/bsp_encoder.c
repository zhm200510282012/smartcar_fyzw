#ifndef HOST_SIL
#include "AI8051U_GPIO.h"
#include "AI8051U_Timer.h"
#endif
#include "bsp_encoder.h"
#include "board_map.h"

static s32 g_left_total;
static s32 g_right_total;

#ifndef HOST_SIL
static s16 read_t3_delta(void)
{
    s16 value;
    u16 raw;

    T4T3M &= ~(1u << 3);
    raw = (u16)(((u16)T3H << 8) | T3L);
    if (gpio_read_pin(P0_5) != 0u) {
        value = (s16)raw;
    } else {
        value = (s16)(0 - (s16)raw);
    }
    T3H = 0u;
    T3L = 0u;
    T4T3M |= (1u << 3);
    return value;
}

static s16 read_t4_delta(void)
{
    s16 value;
    u16 raw;

    T4T3M &= ~(1u << 7);
    raw = (u16)(((u16)T4H << 8) | T4L);
    if (gpio_read_pin(P0_7) != 0u) {
        value = (s16)raw;
    } else {
        value = (s16)(0 - (s16)raw);
    }
    T4H = 0u;
    T4L = 0u;
    T4T3M |= (1u << 7);
    return value;
}
#endif

void bsp_encoder_init(void)
{
    g_left_total = 0l;
    g_right_total = 0l;
#ifndef HOST_SIL
    gpio_init_pin(P0_4, GPIO_Mode_IN_FLOATING);
    gpio_init_pin(P0_5, GPIO_Mode_IN_FLOATING);
    gpio_init_pin(P0_6, GPIO_Mode_IN_FLOATING);
    gpio_init_pin(P0_7, GPIO_Mode_IN_FLOATING);
    T3L = 0u;
    T3H = 0u;
    T4L = 0u;
    T4H = 0u;
    T4T3M |= 0xCCu;
#endif
}

encoder_sample_t bsp_encoder_read(void)
{
    encoder_sample_t e;
    s16 left_delta;
    s16 right_delta;

#ifndef HOST_SIL
    left_delta = read_t3_delta();
    right_delta = read_t4_delta();
#else
    left_delta = 0;
    right_delta = 0;
#endif
    g_left_total += left_delta;
    g_right_total += right_delta;
    e.left_count = 0l;
    e.right_count = 0l;
    e.left_count = g_left_total;
    e.right_count = g_right_total;
    e.left_speed_mm_s = left_delta;
    e.right_speed_mm_s = right_delta;
    e.valid = (BOARD_ENCODER_VERIFIED != 0) ? APP_TRUE : APP_FALSE;
    return e;
}
