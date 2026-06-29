#ifndef TRACK_ROUTE_EVENT_H
#define TRACK_ROUTE_EVENT_H

#include "../App/app_types.h"

typedef struct {
    u8 wall_approach_event;
    u8 crossing_event;
    u8 omega_event;
    u8 hex_loop_event;
    u8 finish_event;
} track_route_event_t;

track_route_event_t track_route_event_none(void);

#endif
