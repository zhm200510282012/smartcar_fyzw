/*
 * 上墙必须经 WALL_APPROACH -> FAN_PRECHARGE -> TRANSITION_UP -> WALL_TRACK。
 * 风机预充压用于给 ESC/风道建立响应时间；下墙 GROUND_RECOVERY 使用 RAMP_DOWN，
 * 避免从墙面回地面时立刻撤掉逻辑附着请求。真实 P2.2 输出仍由物理总开关限制。
 */
#include "track_wall_logic.h"

#if defined(HOST_SIL_LOGICAL_WALL_PROFILE)
#define TRACK_WALL_LOGIC_ALLOWED 1
#else
#define TRACK_WALL_LOGIC_ALLOWED WALL_RUN_ENABLE
#endif

static s16 calibrated_pitch(const attitude_sample_t *attitude)
{
    s32 pitch;
    pitch = ((s32)IMU_PITCH_SIGN * (s32)attitude->pitch_cdeg) +
            (s32)IMU_PITCH_OFFSET_CDEG;
    if (pitch > 32767l) pitch = 32767l;
    if (pitch < -32768l) pitch = -32768l;
    return (s16)pitch;
}

static void enter_state(track_wall_logic_t *logic, track_wall_state_t state)
{
    if (logic->state != state) {
        logic->state = state;
        logic->state_elapsed_ms = 0u;
        logic->transition_confirm_ms = 0u;
        logic->ground_confirm_ms = 0u;
        if (state == TRACK_WALL_TRANSITION_DOWN) {
            logic->transition_down_latched = APP_TRUE;
        }
    }
}

static void fill_output(track_wall_output_t *out, const track_wall_logic_t *logic)
{
    out->state = logic->state;
    out->drive_allowed = APP_TRUE;
    out->finish_ready = logic->finish_ready;
    out->state_elapsed_ms = logic->state_elapsed_ms;

    switch (logic->state) {
    case TRACK_WALL_GROUND_TRACK:
        out->fan_state = FAN_ESC_OFF;
        out->track_mode = TRACK_MODE_STRAIGHT;
        out->speed_limit_mm_s = GROUND_STRAIGHT_SPEED_MM_S;
        break;
    case TRACK_WALL_WALL_APPROACH:
        out->fan_state = FAN_ESC_OFF;
        out->track_mode = TRACK_MODE_TRANSITION;
        out->speed_limit_mm_s = TRANSITION_SPEED_MM_S;
        break;
    case TRACK_WALL_FAN_PRECHARGE:
        out->fan_state = FAN_ESC_PRECHARGE;
        out->track_mode = TRACK_MODE_TRANSITION;
        out->speed_limit_mm_s = TRANSITION_SPEED_MM_S;
        break;
    case TRACK_WALL_TRANSITION_UP:
        out->fan_state = FAN_ESC_HOLD;
        out->track_mode = TRACK_MODE_TRANSITION;
        out->speed_limit_mm_s = TRANSITION_SPEED_MM_S;
        break;
    case TRACK_WALL_WALL_TRACK:
        out->fan_state = FAN_ESC_HOLD;
        out->track_mode = TRACK_MODE_WALL;
        out->speed_limit_mm_s = WALL_SPEED_MM_S;
        break;
    case TRACK_WALL_CYLINDER_TRACK:
        out->fan_state = FAN_ESC_HOLD;
        out->track_mode = TRACK_MODE_CYLINDER;
        out->speed_limit_mm_s = SHARP_CURVE_SPEED_MM_S;
        break;
    case TRACK_WALL_TRANSITION_DOWN:
        out->fan_state = FAN_ESC_HOLD;
        out->track_mode = TRACK_MODE_TRANSITION;
        out->speed_limit_mm_s = TRANSITION_SPEED_MM_S;
        break;
    case TRACK_WALL_GROUND_RECOVERY:
        out->fan_state = FAN_ESC_RAMP_DOWN;
        out->track_mode = TRACK_MODE_RECOVERY;
        out->speed_limit_mm_s = TRANSITION_SPEED_MM_S;
        break;
    case TRACK_WALL_FAILSAFE_HOLD:
    default:
        out->fan_state = FAN_ESC_FAILSAFE_HOLD;
        out->track_mode = TRACK_MODE_LINE_LOST;
        out->speed_limit_mm_s = 0;
        out->drive_allowed = APP_FALSE;
        break;
    }
}

void track_wall_logic_init(track_wall_logic_t *logic)
{
    if (logic == 0) {
        return;
    }
    logic->state = TRACK_WALL_GROUND_TRACK;
    logic->state_elapsed_ms = 0u;
    logic->transition_confirm_ms = 0u;
    logic->ground_confirm_ms = 0u;
    logic->transition_down_latched = APP_FALSE;
    logic->wall_approach_latched = APP_FALSE;
    logic->finish_event_consumed = APP_FALSE;
    logic->wall_cycle_active = APP_FALSE;
    logic->ground_recovery_seen = APP_FALSE;
    logic->previous_wall_approach_event = APP_FALSE;
    logic->finish_ready = APP_FALSE;
}

void track_wall_logic_reset(track_wall_logic_t *logic)
{
    track_wall_logic_init(logic);
}

static u8 wall_related_state(track_wall_state_t state)
{
    return (state == TRACK_WALL_TRANSITION_UP ||
            state == TRACK_WALL_WALL_TRACK ||
            state == TRACK_WALL_CYLINDER_TRACK ||
            state == TRACK_WALL_TRANSITION_DOWN);
}

track_wall_output_t track_wall_logic_update(track_wall_logic_t *logic,
                                            const track_wall_input_t *input)
{
    track_wall_output_t out;
    s16 pitch;

    if (logic == 0 || input == 0) {
        static track_wall_logic_t safe_logic;
        track_wall_logic_init(&safe_logic);
        fill_output(&out, &safe_logic);
        return out;
    }

    logic->state_elapsed_ms = (u16)(logic->state_elapsed_ms + input->dt_ms);
    logic->finish_ready = APP_FALSE;

    if ((input->attitude.fresh == APP_FALSE || input->attitude.id_ok == APP_FALSE) &&
        wall_related_state(logic->state) != APP_FALSE) {
        enter_state(logic, TRACK_WALL_FAILSAFE_HOLD);
        fill_output(&out, logic);
        return out;
    }

    pitch = calibrated_pitch(&input->attitude);

    switch (logic->state) {
    case TRACK_WALL_GROUND_TRACK:
        logic->transition_down_latched = APP_FALSE;
        logic->wall_cycle_active = APP_FALSE;
        logic->wall_approach_latched = APP_FALSE;
        if (TRACK_WALL_LOGIC_ALLOWED != 0 &&
            input->route_event.wall_approach_event != APP_FALSE &&
            logic->previous_wall_approach_event == APP_FALSE) {
            logic->wall_approach_latched = APP_TRUE;
            logic->wall_cycle_active = APP_TRUE;
            logic->ground_recovery_seen = APP_FALSE;
            logic->finish_event_consumed = APP_FALSE;
            enter_state(logic, TRACK_WALL_WALL_APPROACH);
        }
        break;
    case TRACK_WALL_WALL_APPROACH:
        logic->wall_approach_latched = APP_TRUE;
        logic->wall_cycle_active = APP_TRUE;
        enter_state(logic, TRACK_WALL_FAN_PRECHARGE);
        break;
    case TRACK_WALL_FAN_PRECHARGE:
        if (logic->state_elapsed_ms > TRANSITION_TIMEOUT_MS) {
            enter_state(logic, TRACK_WALL_FAILSAFE_HOLD);
        } else if (logic->state_elapsed_ms >= FAN_PRECHARGE_TIME_MS) {
            enter_state(logic, TRACK_WALL_TRANSITION_UP);
        }
        break;
    case TRACK_WALL_TRANSITION_UP:
        if (logic->state_elapsed_ms > TRANSITION_TIMEOUT_MS) {
            enter_state(logic, TRACK_WALL_FAILSAFE_HOLD);
            break;
        }
        if (pitch >= IMU_WALL_ENTER_CDEG) {
            logic->transition_confirm_ms = (u16)(logic->transition_confirm_ms + input->dt_ms);
        } else {
            logic->transition_confirm_ms = 0u;
        }
        if (logic->transition_confirm_ms >= IMU_TRANSITION_CONFIRM_MS) {
            enter_state(logic, TRACK_WALL_WALL_TRACK);
        }
        break;
    case TRACK_WALL_WALL_TRACK:
        if (pitch <= -TRANSITION_PITCH_CDEG) {
            logic->transition_confirm_ms = (u16)(logic->transition_confirm_ms + input->dt_ms);
            if (logic->transition_confirm_ms >= IMU_TRANSITION_CONFIRM_MS) {
                enter_state(logic, TRACK_WALL_CYLINDER_TRACK);
            }
        } else if (pitch <= IMU_WALL_EXIT_CDEG) {
            logic->transition_confirm_ms = (u16)(logic->transition_confirm_ms + input->dt_ms);
            if (logic->transition_confirm_ms >= IMU_TRANSITION_CONFIRM_MS) {
                enter_state(logic, TRACK_WALL_TRANSITION_DOWN);
            }
        } else {
            logic->transition_confirm_ms = 0u;
        }
        break;
    case TRACK_WALL_CYLINDER_TRACK:
        if (pitch >= IMU_WALL_ENTER_CDEG) {
            logic->transition_confirm_ms = (u16)(logic->transition_confirm_ms + input->dt_ms);
            if (logic->transition_confirm_ms >= IMU_TRANSITION_CONFIRM_MS) {
                enter_state(logic, TRACK_WALL_WALL_TRACK);
            }
        } else if (pitch <= IMU_WALL_EXIT_CDEG && pitch > -TRANSITION_PITCH_CDEG) {
            logic->transition_confirm_ms = (u16)(logic->transition_confirm_ms + input->dt_ms);
            if (logic->transition_confirm_ms >= IMU_TRANSITION_CONFIRM_MS) {
                enter_state(logic, TRACK_WALL_TRANSITION_DOWN);
            }
        } else {
            logic->transition_confirm_ms = 0u;
        }
        break;
    case TRACK_WALL_TRANSITION_DOWN:
        if (pitch >= IMU_WALL_ENTER_CDEG) {
            enter_state(logic, TRACK_WALL_FAILSAFE_HOLD);
        } else if (pitch <= GROUND_PITCH_MAX_CDEG) {
            logic->ground_confirm_ms = (u16)(logic->ground_confirm_ms + input->dt_ms);
        } else {
            logic->ground_confirm_ms = 0u;
        }
        if (logic->ground_confirm_ms >= IMU_GROUND_CONFIRM_MS) {
            enter_state(logic, TRACK_WALL_GROUND_RECOVERY);
        }
        break;
    case TRACK_WALL_GROUND_RECOVERY:
        logic->ground_recovery_seen = APP_TRUE;
        if (input->route_event.finish_event != APP_FALSE &&
            logic->finish_event_consumed == APP_FALSE) {
            logic->finish_event_consumed = APP_TRUE;
            logic->finish_ready = APP_TRUE;
        }
        if (pitch <= GROUND_PITCH_MAX_CDEG) {
            logic->ground_confirm_ms = (u16)(logic->ground_confirm_ms + input->dt_ms);
        } else {
            logic->ground_confirm_ms = 0u;
        }
        if (logic->ground_confirm_ms >= (u16)(IMU_GROUND_CONFIRM_MS * 2u)) {
            logic->wall_cycle_active = APP_FALSE;
            enter_state(logic, TRACK_WALL_GROUND_TRACK);
        }
        break;
    case TRACK_WALL_FAILSAFE_HOLD:
    default:
        break;
    }

    logic->previous_wall_approach_event = input->route_event.wall_approach_event;
    fill_output(&out, logic);
    return out;
}
