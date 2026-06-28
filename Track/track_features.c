#include "track_features.h"

u8 track_features_detect_transition(const emag_sample_t *emag)
{
    if (emag == 0) return APP_FALSE;
    if (emag->valid == APP_FALSE) return APP_FALSE;
    if (emag->signal_quality < LINE_QUALITY_MIN) return APP_FALSE;
    return APP_TRUE;
}
