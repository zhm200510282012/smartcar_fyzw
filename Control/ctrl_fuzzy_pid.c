#include "ctrl_fuzzy_pid.h"
#include "ctrl_signal.h"

static const s16 g_centers[FUZZY_TERM_COUNT] = {
    -1000, -500, 0, 500, 1000
};

static const s16 g_rule_kp_permille[FUZZY_TERM_COUNT][FUZZY_TERM_COUNT] = {
    {900, 780, 640, 780, 900},
    {680, 520, 340, 520, 680},
    {280, 120,   0, 120, 280},
    {680, 520, 340, 520, 680},
    {900, 780, 640, 780, 900}
};

static const s16 g_rule_ki_permille[FUZZY_TERM_COUNT][FUZZY_TERM_COUNT] = {
    {-1000, -900, -700, -900, -1000},
    { -800, -550, -250, -550,  -800},
    { -500, -180,  200, -180,  -500},
    { -800, -550, -250, -550,  -800},
    {-1000, -900, -700, -900, -1000}
};

static const s16 g_rule_kd_permille[FUZZY_TERM_COUNT][FUZZY_TERM_COUNT] = {
    {950, 760, 520, 760, 950},
    {850, 620, 360, 620, 850},
    {720, 450, 160, 450, 720},
    {850, 620, 360, 620, 850},
    {950, 760, 520, 760, 950}
};

static s16 clamp_gain(s16 value, s16 min_value, s16 max_value)
{
    return ctrl_signal_clamp_s16(value, min_value, max_value);
}

static s16 clamp_permille(s16 value)
{
    if (value > FUZZY_WEIGHT_SCALE) return FUZZY_WEIGHT_SCALE;
    if (value < -FUZZY_WEIGHT_SCALE) return -FUZZY_WEIGHT_SCALE;
    return value;
}

static s16 scale_rule_value(s16 permille, s16 limit)
{
    s32 value;
    value = ((s32)permille * (s32)limit) / (s32)FUZZY_WEIGHT_SCALE;
    return (s16)value;
}

s16 ctrl_fuzzy_pid_normalize(s16 value, s16 scale)
{
    s32 normalized;
    if (scale <= 0) {
        return 0;
    }
    normalized = ((s32)value * (s32)FUZZY_E_SCALE) / (s32)scale;
    if (normalized > FUZZY_E_SCALE) normalized = FUZZY_E_SCALE;
    if (normalized < -FUZZY_E_SCALE) normalized = -FUZZY_E_SCALE;
    return (s16)normalized;
}

void ctrl_fuzzy_pid_membership(s16 normalized, fuzzy_membership_t *out)
{
    u8 index;
    s16 left;
    s16 right;
    s32 numerator;

    if (out == 0) {
        return;
    }

    if (normalized <= g_centers[0]) {
        out->term_a = FUZZY_NB;
        out->weight_a = FUZZY_WEIGHT_SCALE;
        out->term_b = FUZZY_NB;
        out->weight_b = 0;
        return;
    }
    if (normalized >= g_centers[FUZZY_TERM_COUNT - 1u]) {
        out->term_a = FUZZY_PB;
        out->weight_a = FUZZY_WEIGHT_SCALE;
        out->term_b = FUZZY_PB;
        out->weight_b = 0;
        return;
    }

    for (index = 0u; index < (FUZZY_TERM_COUNT - 1u); index++) {
        left = g_centers[index];
        right = g_centers[index + 1u];
        if (normalized >= left && normalized <= right) {
            numerator = ((s32)normalized - (s32)left) * (s32)FUZZY_WEIGHT_SCALE;
            out->term_a = (s16)index;
            out->term_b = (s16)(index + 1u);
            out->weight_b = (s16)(numerator / ((s32)right - (s32)left));
            out->weight_a = (s16)(FUZZY_WEIGHT_SCALE - out->weight_b);
            if (out->weight_a < 0) out->weight_a = 0;
            if (out->weight_b < 0) out->weight_b = 0;
            return;
        }
    }

    out->term_a = FUZZY_ZO;
    out->weight_a = FUZZY_WEIGHT_SCALE;
    out->term_b = FUZZY_ZO;
    out->weight_b = 0;
}

void ctrl_fuzzy_pid_rule_value(u8 e_term,
                               u8 de_term,
                               const fuzzy_pid_base_t *base,
                               fuzzy_pid_adjust_t *adjust)
{
    s16 kp_permille;
    s16 ki_permille;
    s16 kd_permille;

    if (adjust == 0) {
        return;
    }
    if (base == 0 || e_term >= FUZZY_TERM_COUNT || de_term >= FUZZY_TERM_COUNT) {
        adjust->dkp = 0;
        adjust->dki = 0;
        adjust->dkd = 0;
        return;
    }

    kp_permille = clamp_permille(g_rule_kp_permille[e_term][de_term]);
    ki_permille = clamp_permille(g_rule_ki_permille[e_term][de_term]);
    kd_permille = clamp_permille(g_rule_kd_permille[e_term][de_term]);
    adjust->dkp = scale_rule_value(kp_permille, base->kp_adjust_limit);
    adjust->dki = scale_rule_value(ki_permille, base->ki_adjust_limit);
    adjust->dkd = scale_rule_value(kd_permille, base->kd_adjust_limit);
}

void ctrl_fuzzy_pid_eval(s16 e_norm,
                         s16 de_norm,
                         const fuzzy_pid_base_t *base,
                         fuzzy_pid_gain_t *gain,
                         fuzzy_pid_adjust_t *adjust)
{
    fuzzy_membership_t e_mem;
    fuzzy_membership_t de_mem;
    fuzzy_pid_adjust_t rule;
    s32 sum_weight;
    s32 sum_kp;
    s32 sum_ki;
    s32 sum_kd;
    s32 weight;
    u8 e_index;
    u8 de_index;
    s16 e_terms[2];
    s16 e_weights[2];
    s16 de_terms[2];
    s16 de_weights[2];

    if (gain == 0 || adjust == 0) {
        return;
    }

    gain->kp = 0;
    gain->ki = 0;
    gain->kd = 0;
    adjust->dkp = 0;
    adjust->dki = 0;
    adjust->dkd = 0;

    if (base == 0) {
        return;
    }

    ctrl_fuzzy_pid_membership(e_norm, &e_mem);
    ctrl_fuzzy_pid_membership(de_norm, &de_mem);

    e_terms[0] = e_mem.term_a;
    e_terms[1] = e_mem.term_b;
    e_weights[0] = e_mem.weight_a;
    e_weights[1] = e_mem.weight_b;
    de_terms[0] = de_mem.term_a;
    de_terms[1] = de_mem.term_b;
    de_weights[0] = de_mem.weight_a;
    de_weights[1] = de_mem.weight_b;

    sum_weight = 0l;
    sum_kp = 0l;
    sum_ki = 0l;
    sum_kd = 0l;

    for (e_index = 0u; e_index < 2u; e_index++) {
        if (e_weights[e_index] == 0) {
            continue;
        }
        for (de_index = 0u; de_index < 2u; de_index++) {
            if (de_weights[de_index] == 0) {
                continue;
            }
            weight = (s32)e_weights[e_index];
            if ((s32)de_weights[de_index] < weight) {
                weight = (s32)de_weights[de_index];
            }
            ctrl_fuzzy_pid_rule_value((u8)e_terms[e_index], (u8)de_terms[de_index], base, &rule);
            sum_weight += weight;
            sum_kp += weight * (s32)rule.dkp;
            sum_ki += weight * (s32)rule.dki;
            sum_kd += weight * (s32)rule.dkd;
        }
    }

    if (sum_weight > 0l) {
        adjust->dkp = (s16)(sum_kp / sum_weight);
        adjust->dki = (s16)(sum_ki / sum_weight);
        adjust->dkd = (s16)(sum_kd / sum_weight);
    }

    adjust->dkp = ctrl_signal_clamp_s16(adjust->dkp, (s16)-base->kp_adjust_limit, base->kp_adjust_limit);
    adjust->dki = ctrl_signal_clamp_s16(adjust->dki, (s16)-base->ki_adjust_limit, base->ki_adjust_limit);
    adjust->dkd = ctrl_signal_clamp_s16(adjust->dkd, (s16)-base->kd_adjust_limit, base->kd_adjust_limit);

    gain->kp = clamp_gain((s16)(base->base_kp + adjust->dkp), FUZZY_KP_MIN, FUZZY_KP_MAX);
    gain->ki = clamp_gain((s16)(base->base_ki + adjust->dki), FUZZY_KI_MIN, FUZZY_KI_MAX);
    gain->kd = clamp_gain((s16)(base->base_kd + adjust->dkd), FUZZY_KD_MIN, FUZZY_KD_MAX);
}
