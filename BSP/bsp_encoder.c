#ifndef HOST_SIL
#include "AI8051U_GPIO.h"
#include "AI8051U_Timer.h"
#endif
#include "bsp_encoder.h"
#include "../App/app_config.h"
#include "../App/app_memory.h"
#include "board_map.h"

static s32 g_left_total;
static s32 g_right_total;
static encoder_sample_t APP_XDATA g_last_sample;

#ifdef HOST_SIL
static s16 g_host_left_delta;
static s16 g_host_right_delta;
static app_bool_t g_host_valid;
#endif

static u8 encoder_scale_valid(void)
{
    return (ENCODER_COUNTS_PER_WHEEL_REV > 0L &&
            WHEEL_CIRCUMFERENCE_MM > 0L) ? APP_TRUE : APP_FALSE;
}

static s16 counts_per_second(s16 delta_counts)
{
    s32 cps;

    if (ENCODER_SAMPLE_PERIOD_MS == 0u) {
        return 0;
    }
    cps = ((s32)delta_counts * 1000l) / (s32)ENCODER_SAMPLE_PERIOD_MS;
    if (cps > 32767l) {
        cps = 32767l;
    }
    if (cps < -32768l) {
        cps = -32768l;
    }
    return (s16)cps;
}

#if (ENCODER_COUNTS_PER_WHEEL_REV > 0L) && (WHEEL_CIRCUMFERENCE_MM > 0L)
static s16 speed_mm_s_from_counts(s16 counts_s)
{
    s32 speed;

    speed = ((s32)counts_s * (s32)WHEEL_CIRCUMFERENCE_MM) /
            (s32)ENCODER_COUNTS_PER_WHEEL_REV;
    if (speed > 32767l) {
        speed = 32767l;
    }
    if (speed < -32768l) {
        speed = -32768l;
    }
    return (s16)speed;
}
#else
static s16 speed_mm_s_from_counts(void)
{
    return 0;
}
#endif

#if (ENCODER_COUNTS_PER_WHEEL_REV > 0L) && (WHEEL_CIRCUMFERENCE_MM > 0L)
static s32 distance_mm_from_counts(s32 counts)
{
    return (counts * (s32)WHEEL_CIRCUMFERENCE_MM) /
           (s32)ENCODER_COUNTS_PER_WHEEL_REV;
}
#else
static s32 distance_mm_from_counts(void)
{
    return 0l;
}
#endif

static void clear_encoder_sample(encoder_sample_t *e)
{
    e->left_count = 0l;
    e->right_count = 0l;
    e->left_delta_counts = 0;
    e->right_delta_counts = 0;
    e->left_speed_counts_per_s = 0;
    e->right_speed_counts_per_s = 0;
    e->left_speed_mm_s = 0;
    e->right_speed_mm_s = 0;
    e->left_distance_mm = 0l;
    e->right_distance_mm = 0l;
    e->speed_mm_s_valid = APP_FALSE;
    e->progress_mm_valid = APP_FALSE;
    e->valid = APP_FALSE;
}

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
    clear_encoder_sample(&g_last_sample);
#ifdef HOST_SIL
    g_host_left_delta = 0;
    g_host_right_delta = 0;
    g_host_valid = APP_FALSE;
#endif
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
    left_delta = g_host_left_delta;
    right_delta = g_host_right_delta;
#endif
    g_left_total += left_delta;
    g_right_total += right_delta;
    clear_encoder_sample(&e);
    e.left_count = g_left_total;
    e.right_count = g_right_total;
    e.left_delta_counts = left_delta;
    e.right_delta_counts = right_delta;
    e.left_speed_counts_per_s = counts_per_second(left_delta);
    e.right_speed_counts_per_s = counts_per_second(right_delta);
    e.speed_mm_s_valid = encoder_scale_valid();
    e.progress_mm_valid = encoder_scale_valid();
#if (ENCODER_COUNTS_PER_WHEEL_REV > 0L) && (WHEEL_CIRCUMFERENCE_MM > 0L)
    e.left_speed_mm_s = speed_mm_s_from_counts(e.left_speed_counts_per_s);
    e.right_speed_mm_s = speed_mm_s_from_counts(e.right_speed_counts_per_s);
    e.left_distance_mm = distance_mm_from_counts(e.left_count);
    e.right_distance_mm = distance_mm_from_counts(e.right_count);
#else
    e.left_speed_mm_s = speed_mm_s_from_counts();
    e.right_speed_mm_s = speed_mm_s_from_counts();
    e.left_distance_mm = distance_mm_from_counts();
    e.right_distance_mm = distance_mm_from_counts();
#endif
#ifdef HOST_SIL
    e.valid = g_host_valid;
#else
    e.valid = (BOARD_ENCODER_VERIFIED != 0) ? APP_TRUE : APP_FALSE;
#endif
    g_last_sample = e;
    return e;
}

encoder_sample_t bsp_encoder_last_sample(void)
{
    return g_last_sample;
}

#ifdef HOST_SIL
void bsp_encoder_host_set_delta_counts(s16 left_delta, s16 right_delta, app_bool_t valid)
{
    g_host_left_delta = left_delta;
    g_host_right_delta = right_delta;
    g_host_valid = valid;
}
#endif
