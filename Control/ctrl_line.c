/*
 * 模块职责：五路电磁 A-E 加权重心循迹。
 * 输入：norm[0..4]，顺序为车头方向从左到右 A/B/C/D/E。
 * 输出：line_error、line_quality、line_lost；line_error 正负方向由 LINE_DIRECTION_SIGN 统一翻转。
 * 单位：权重内部放大 1000 倍，过滤后的误差由 scheduler 继续求变化率。
 */
#include "ctrl_line.h"

static const s16 g_emag_weights[EMAG_CHANNEL_COUNT] = {
    -2000, -1000, 0, 1000, 2000
};

static void mark_lost(emag_sample_t *sample)
{
    sample->line_error = 0;
    sample->line_quality = 0u;
    sample->signal_quality = 0u;
    sample->line_lost = APP_TRUE;
    sample->valid = APP_FALSE;
}

emag_sample_t ctrl_line_update(emag_sample_t input)
{
    u8 i;
    u32 sum;
    s32 weighted;
    s32 error;

    if (input.channel_count < EMAG_CHANNEL_COUNT || input.valid == APP_FALSE) {
        mark_lost(&input);
        return input;
    }

    sum = 0ul;
    weighted = 0l;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        sum += (u32)input.norm[i];
        weighted += (s32)g_emag_weights[i] * (s32)input.norm[i];
    }

    if (sum < LINE_VALID_SUM_MIN) {
        mark_lost(&input);
        return input;
    }

    error = (weighted * (s32)LINE_WEIGHT_SCALE) / (s32)sum;
#if LINE_DIRECTION_SIGN < 0
    error = -error;
#endif
    if (error > 32767l) error = 32767l;
    if (error < -32768l) error = -32768l;
    input.line_error = (s16)error;
    input.line_quality = (u16)sum;
    input.signal_quality = input.line_quality;
    input.line_lost = APP_FALSE;
    input.valid = APP_TRUE;
    return input;
}

void ctrl_line_filter_init(line_filter_state_t *state)
{
    if (state == 0) {
        return;
    }
    state->valid = APP_FALSE;
    state->previous_filtered = 0;
}

void ctrl_line_filter_reset(line_filter_state_t *state)
{
    ctrl_line_filter_init(state);
}

void ctrl_line_filter_update(line_filter_state_t *state,
                             const emag_sample_t *sample,
                             s16 *filtered_error,
                             s16 *error_rate)
{
    s16 filtered;
    s16 previous;
    s32 accum;

    if (filtered_error != 0) {
        *filtered_error = 0;
    }
    if (error_rate != 0) {
        *error_rate = 0;
    }
    if (state == 0 || sample == 0) {
        return;
    }
    if (sample->valid == APP_FALSE || sample->line_lost != APP_FALSE) {
        ctrl_line_filter_reset(state);
        return;
    }

    previous = state->previous_filtered;
    if (state->valid == APP_FALSE) {
        filtered = sample->line_error;
        state->valid = APP_TRUE;
    } else {
        accum = ((s32)previous * (s32)LINE_FILTER_ALPHA) +
                ((s32)sample->line_error * (s32)(LINE_FILTER_DENOM - LINE_FILTER_ALPHA));
        filtered = (s16)(accum / (s32)LINE_FILTER_DENOM);
    }

    if (filtered_error != 0) {
        *filtered_error = filtered;
    }
    if (error_rate != 0) {
        *error_rate = (s16)(filtered - previous);
    }
    state->previous_filtered = filtered;
}
