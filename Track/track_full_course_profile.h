#ifndef TRACK_FULL_COURSE_PROFILE_H
#define TRACK_FULL_COURSE_PROFILE_H

#include "../App/app_types.h"
#include "../App/app_config.h"
#include "track_route_event.h"

/*
 * 模块职责：把地面循迹、路线事件和墙面状态统一映射为完整赛道赛段。
 * 输入：line_error/line_rate、route_event、track_mode、app_state、wall_state。
 * 输出：当前 track_course_segment_t 与已经过的赛段位图，供调度和 Host-SIL 验证。
 * 单位：线误差为 ctrl_line 的加权重心单位，距离字段保留为 mm。
 */

typedef struct {
    track_course_segment_t segment;
    u32 seen_mask;
} track_full_course_profile_t;

typedef struct {
    track_route_event_t route_event;
    track_wall_state_t wall_state;
    track_mode_t track_mode;
    app_state_t app_state;
    s16 line_error_filtered;
    s16 line_error_rate;
    u16 line_quality;
    s32 progress_distance_mm;
} track_full_course_input_t;

void track_full_course_profile_init(track_full_course_profile_t *profile);
track_course_segment_t track_full_course_profile_update(track_full_course_profile_t *profile,
                                                        const track_full_course_input_t *input);
u32 track_full_course_profile_seen_mask(const track_full_course_profile_t *profile);

#endif
