#include "track_features.h"

u8 track_features_detect_transition(const emag_sample_t *emag)
{
    if (emag == 0) return APP_FALSE;
    if (emag->valid == APP_FALSE) return APP_FALSE;
    if (emag->signal_quality < LINE_QUALITY_MIN) return APP_FALSE;
    return APP_TRUE;
}

u8 track_features_detect_crossing(const emag_sample_t *emag)
{
    u8 index;
    u16 threshold;

    if (emag == 0) return APP_FALSE;
    if (emag->valid == APP_FALSE) return APP_FALSE;
    if (emag->channel_count < 5u) return APP_FALSE;
    if (emag->signal_quality < TRACK_CROSS_QUALITY_MIN) return APP_FALSE;

    threshold = (u16)(emag->signal_quality / 8u);
    for (index = 0u; index < 5u; index++) {
        if (emag->norm[index] < threshold) {
            return APP_FALSE;
        }
    }
    return APP_TRUE;
}

u8 track_features_detect_hex_loop(const emag_sample_t *emag)
{
    if (emag == 0) return APP_FALSE;
    if (emag->valid == APP_FALSE) return APP_FALSE;
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
