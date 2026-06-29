#ifndef TRACK_ROUTE_EVENT_H
#define TRACK_ROUTE_EVENT_H

#include "../App/app_types.h"
#include "../App/app_config.h"

/*
 * 模块职责：把外部路线事件统一成 track_route_event_t。
 * 输入：Host-SIL 注入、未来串口/按键手动注入、编码器累计里程。
 * 输出：十字、Omega、环岛、上墙入口、终点等离散事件。
 * 关系：scheduler 只消费本模块输出，不在控制链中猜测真实赛道顺序。
 */

typedef struct {
    u8 wall_approach_event;
    u8 crossing_event;
    u8 omega_event;
    u8 hex_loop_event;
    u8 finish_event;
} track_route_event_t;

typedef enum {
    ROUTE_PROGRESS_DISABLED = 0,
    ROUTE_PROGRESS_READY,
    ROUTE_PROGRESS_UNCALIBRATED,
    ROUTE_PROGRESS_ENCODER_INVALID
} route_progress_status_t;

track_route_event_t track_route_event_none(void);
void track_route_event_manual_set(const track_route_event_t *event);
track_route_event_t track_route_event_manual_get(void);
void track_route_event_manual_clear(void);
track_route_event_t track_route_event_from_progress(const encoder_sample_t *encoder);
route_progress_status_t track_route_event_progress_status(void);
track_route_event_t track_route_event_select(u8 source,
                                             const track_route_event_t *host_event,
                                             const encoder_sample_t *encoder);

#endif
