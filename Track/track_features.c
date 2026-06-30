#include "track_features.h"
#include "../App/app_memory.h"

static u8 APP_XDATA g_active_mask;
static u8 APP_XDATA g_baseline_count;
static u16 APP_XDATA g_confirm_ms;
static u16 APP_XDATA g_release_ms;
static u16 APP_XDATA g_cooldown_ms;
static u16 APP_XDATA g_burst_hold_ms;
static app_bool_t APP_XDATA g_burst_active;
static app_bool_t APP_XDATA g_armed;
static track_special_candidate_t APP_XDATA g_candidate;

static u8 popcount5(u8 value)
{
    u8 count;
    u8 i;

    count = 0u;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        if ((value & (u8)(1u << i)) != 0u) {
            count++;
        }
    }
    return count;
}

static u8 update_active_mask(const emag_sample_t *emag)
{
    u8 index;
    u8 channel_mask;

    for (index = 0u; index < EMAG_CHANNEL_COUNT; index++) {
        channel_mask = (u8)(1u << index);
        if ((g_active_mask & channel_mask) == 0u) {
            if (emag->norm[index] >= ELEMENT_ACTIVE_ENTER_NORM) {
                g_active_mask = (u8)(g_active_mask | channel_mask);
            }
        } else {
            if (emag->norm[index] <= ELEMENT_ACTIVE_EXIT_NORM) {
                g_active_mask = (u8)(g_active_mask & (u8)(~channel_mask));
            }
        }
    }
    return popcount5(g_active_mask);
}

static void refresh_baseline(u8 active_count)
{
    if (active_count <= ELEMENT_BURST_BASELINE_MAX_ACTIVE) {
        g_baseline_count = active_count;
    }
}

static track_special_candidate_t classify_candidate(u8 active_mask, u8 active_count)
{
    u8 left_outer;
    u8 right_outer;

    if (ELEMENT_SPECIAL_DIRECTION_CONFIGURED == 0) {
        return TRACK_SPECIAL_ELEMENT_CANDIDATE;
    }

    left_outer = (active_mask & 0x03u);
    right_outer = (active_mask & 0x18u);
    if (active_count >= 4u) {
        return TRACK_SPECIAL_CROSSING_CANDIDATE;
    }
    if (g_burst_hold_ms >= ELEMENT_RING_HOLD_MS && active_count >= ELEMENT_BURST_MIN_ACTIVE) {
        return TRACK_SPECIAL_RING_CANDIDATE;
    }
    if ((left_outer != 0u && right_outer == 0u) ||
        (right_outer != 0u && left_outer == 0u)) {
        return TRACK_SPECIAL_RIGHT_ANGLE_CANDIDATE;
    }
    return TRACK_SPECIAL_ELEMENT_CANDIDATE;
}

void track_features_reset(void)
{
    g_active_mask = 0u;
    g_baseline_count = 0u;
    g_confirm_ms = 0u;
    g_release_ms = 0u;
    g_cooldown_ms = 0u;
    g_burst_hold_ms = 0u;
    g_burst_active = APP_FALSE;
    g_armed = APP_TRUE;
    g_candidate = TRACK_SPECIAL_NONE;
}

track_element_feature_t track_features_update_elements(const emag_sample_t *emag, u16 dt_ms)
{
    track_element_feature_t feature;
    u8 active_count;
    s16 rise;
    app_bool_t burst_condition;

    feature.active_element_mask = 0u;
    feature.active_element_count = 0u;
    feature.baseline_element_count = g_baseline_count;
    feature.element_count_rise = 0;
    feature.element_burst = APP_FALSE;
    feature.element_burst_rising_edge = APP_FALSE;
    feature.element_burst_armed = g_armed;

    if (emag == 0 || emag->valid == APP_FALSE || emag->channel_count < EMAG_CHANNEL_COUNT) {
        g_confirm_ms = 0u;
        g_release_ms = 0u;
        g_burst_active = APP_FALSE;
        g_candidate = TRACK_SPECIAL_NONE;
        return feature;
    }

    active_count = update_active_mask(emag);
    refresh_baseline(active_count);
    rise = (s16)active_count - (s16)g_baseline_count;
    if (rise > 127) rise = 127;
    if (rise < -128) rise = -128;

    feature.active_element_mask = g_active_mask;
    feature.active_element_count = active_count;
    feature.baseline_element_count = g_baseline_count;
    feature.element_count_rise = (s8)rise;

    if (g_cooldown_ms > 0u) {
        if (g_cooldown_ms > dt_ms) {
            g_cooldown_ms = (u16)(g_cooldown_ms - dt_ms);
        } else {
            g_cooldown_ms = 0u;
        }
    }

    burst_condition = APP_FALSE;
#if ELEMENT_BURST_ENABLE != 0
    if (active_count >= ELEMENT_BURST_MIN_ACTIVE &&
        rise >= ELEMENT_BURST_MIN_RISE &&
        emag->line_quality >= ELEMENT_BURST_MIN_QUALITY) {
        burst_condition = APP_TRUE;
    }
#endif

    if (burst_condition != APP_FALSE && g_armed != APP_FALSE && g_cooldown_ms == 0u) {
        g_confirm_ms = (u16)(g_confirm_ms + dt_ms);
        g_release_ms = 0u;
        if (g_confirm_ms >= ELEMENT_BURST_CONFIRM_MS) {
            if (g_burst_active == APP_FALSE) {
                feature.element_burst_rising_edge = APP_TRUE;
            }
            g_burst_active = APP_TRUE;
            g_armed = APP_FALSE;
            g_burst_hold_ms = (u16)(g_burst_hold_ms + dt_ms);
            g_candidate = classify_candidate(g_active_mask, active_count);
        }
    } else {
        g_confirm_ms = 0u;
        if (active_count <= ELEMENT_BURST_BASELINE_MAX_ACTIVE) {
            g_release_ms = (u16)(g_release_ms + dt_ms);
        } else {
            g_release_ms = 0u;
        }
        if (g_release_ms >= ELEMENT_BURST_RELEASE_MS) {
            g_burst_active = APP_FALSE;
            g_armed = APP_TRUE;
            g_burst_hold_ms = 0u;
            if (g_cooldown_ms == 0u) {
                g_cooldown_ms = ELEMENT_BURST_COOLDOWN_MS;
            }
            g_candidate = TRACK_SPECIAL_NONE;
        }
    }

    if (g_burst_active != APP_FALSE && burst_condition != APP_FALSE) {
        g_burst_hold_ms = (u16)(g_burst_hold_ms + dt_ms);
        if (g_candidate == TRACK_SPECIAL_ELEMENT_CANDIDATE) {
            g_candidate = classify_candidate(g_active_mask, active_count);
        }
    }

    feature.element_burst = g_burst_active;
    feature.element_burst_armed = g_armed;
    return feature;
}

track_special_candidate_t track_features_special_candidate(void)
{
    return g_candidate;
}

u8 track_features_detect_transition(const emag_sample_t *emag)
{
    if (emag == 0) return APP_FALSE;
    if (emag->valid == APP_FALSE) return APP_FALSE;
    if (emag->line_quality < LINE_QUALITY_MIN) return APP_FALSE;
    return APP_TRUE;
}

u8 track_features_detect_crossing(const emag_sample_t *emag)
{
    if (emag == 0) return APP_FALSE;
    if (emag->valid == APP_FALSE) return APP_FALSE;
    if (track_features_special_candidate() == TRACK_SPECIAL_CROSSING_CANDIDATE) {
        return APP_TRUE;
    }
    return APP_FALSE;
}

u8 track_features_detect_hex_loop(const emag_sample_t *emag)
{
    if (emag == 0) return APP_FALSE;
    if (emag->valid == APP_FALSE) return APP_FALSE;
    if (track_features_special_candidate() == TRACK_SPECIAL_RING_CANDIDATE) {
        return APP_TRUE;
    }
    return APP_FALSE;
}

u8 track_features_detect_seesaw(const attitude_sample_t *attitude)
{
    if (attitude == 0) return APP_FALSE;
    if (attitude->fresh == APP_FALSE) return APP_FALSE;
    if (attitude->pitch_cdeg > 1600 || attitude->pitch_cdeg < -1600) {
        return APP_TRUE;
    }
    return APP_FALSE;
}
