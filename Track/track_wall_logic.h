#ifndef TRACK_WALL_LOGIC_H
#define TRACK_WALL_LOGIC_H

/*
 * 模块职责：上墙/墙面/下墙逻辑状态机。
 * 输入：route_event、IMU pitch、丢线和附着风险。
 * 输出：墙面状态、风机逻辑状态、速度限制和 drive_allowed。
 * 关系：真实上墙由 WALL_RUN_ENABLE 控制；Host-SIL 可用逻辑墙面 profile 验证时序。
 */

#include "../App/app_types.h"
#include "../App/app_config.h"
#include "track_route_event.h"

typedef struct {
    track_wall_state_t state;
    u16 state_elapsed_ms;
    u16 transition_confirm_ms;
    u16 ground_confirm_ms;
    u8 transition_down_latched;
    u8 wall_approach_latched;
    u8 finish_event_consumed;
    u8 wall_cycle_active;
    u8 ground_recovery_seen;
    u8 previous_wall_approach_event;
    u8 finish_ready;
} track_wall_logic_t;

typedef struct {
    track_route_event_t route_event;
    attitude_sample_t attitude;
    u8 line_lost;
    u16 adhesion_risk;
    u16 dt_ms;
} track_wall_input_t;

typedef struct {
    track_wall_state_t state;
    fan_esc_state_t fan_state;
    track_mode_t track_mode;
    s16 speed_limit_mm_s;
    u8 drive_allowed;
    u8 finish_ready;
    u16 state_elapsed_ms;
} track_wall_output_t;

void track_wall_logic_init(track_wall_logic_t *logic);
void track_wall_logic_reset(track_wall_logic_t *logic);
track_wall_output_t track_wall_logic_update(track_wall_logic_t *logic,
                                            const track_wall_input_t *input);

#endif
