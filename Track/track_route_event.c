#include "track_route_event.h"

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
