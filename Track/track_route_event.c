#include "track_route_event.h"

static track_route_event_t g_manual_event;

track_route_event_t track_route_event_none(void)
{
    track_route_event_t event;
    event.wall_approach_event = APP_FALSE;
    event.crossing_event = APP_FALSE;
    event.omega_event = APP_FALSE;
    event.hex_loop_event = APP_FALSE;
    event.finish_event = APP_FALSE;
    return event;
}

void track_route_event_manual_set(const track_route_event_t *event)
{
    if (event == 0) {
        g_manual_event = track_route_event_none();
        return;
    }
    g_manual_event = *event;
}

track_route_event_t track_route_event_manual_get(void)
{
    return g_manual_event;
}

void track_route_event_manual_clear(void)
{
    g_manual_event = track_route_event_none();
}

#if ROUTE_PROGRESS_SCRIPT_ENABLE != 0
static s32 abs_s32_local(s32 value)
{
    if (value < 0l) {
        return -value;
    }
    return value;
}

static s32 progress_distance_mm(const encoder_sample_t *encoder)
{
    s32 left;
    s32 right;
    if (encoder == 0 || encoder->valid == APP_FALSE) {
        return 0l;
    }
    left = abs_s32_local(encoder->left_count);
    right = abs_s32_local(encoder->right_count);
    return (left + right) / 2l;
}
#endif

track_route_event_t track_route_event_from_progress(const encoder_sample_t *encoder)
{
    track_route_event_t event;
#if ROUTE_PROGRESS_SCRIPT_ENABLE != 0
    s32 distance_mm;
#endif

    event = track_route_event_none();
#if ROUTE_PROGRESS_SCRIPT_ENABLE != 0
    distance_mm = progress_distance_mm(encoder);
    if (ROUTE_WALL_APPROACH_DISTANCE_MM > 0L &&
        distance_mm >= ROUTE_WALL_APPROACH_DISTANCE_MM &&
        (ROUTE_WALL_EXIT_DISTANCE_MM <= 0L ||
         distance_mm < ROUTE_WALL_EXIT_DISTANCE_MM)) {
        event.wall_approach_event = APP_TRUE;
    }
    if (ROUTE_FINISH_DISTANCE_MM > 0L &&
        distance_mm >= ROUTE_FINISH_DISTANCE_MM) {
        event.finish_event = APP_TRUE;
    }
#else
    if (encoder != 0 && encoder->valid == APP_FALSE) {
        event = track_route_event_none();
    }
#endif
    return event;
}

track_route_event_t track_route_event_select(u8 source,
                                             const track_route_event_t *host_event,
                                             const encoder_sample_t *encoder)
{
    switch (source) {
    case ROUTE_EVENT_SOURCE_HOST_SIL:
        if (host_event != 0) {
            return *host_event;
        }
        return track_route_event_none();
    case ROUTE_EVENT_SOURCE_MANUAL_INJECT:
        return track_route_event_manual_get();
    case ROUTE_EVENT_SOURCE_PROGRESS_SCRIPT:
        return track_route_event_from_progress(encoder);
    case ROUTE_EVENT_SOURCE_NONE:
    default:
        return track_route_event_none();
    }
}
