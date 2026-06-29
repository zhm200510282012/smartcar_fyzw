#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_bsp.h"
#include "../../App/app_scheduler.h"
#include "../../App/app_state_machine.h"
#include "../../App/app_telemetry.h"
#include "../../BSP/bsp_drive.h"
#include "../../BSP/bsp_emag.h"
#include "../../BSP/bsp_encoder.h"
#include "../../BSP/bsp_imu.h"
#include "../../BSP/bsp_power.h"
#include "../../BSP/bsp_steering.h"
#include "../../BSP/bsp_suction.h"
#include "../../BSP/bsp_timebase.h"
#include "../../BSP/bsp_ui.h"

static void init_system(app_context_t *ctx)
{
    host_bsp_reset();
    bsp_timebase_init();
    bsp_emag_init();
    bsp_imu_init();
    bsp_encoder_init();
    bsp_power_init();
    bsp_ui_init();
    bsp_drive_init();
    bsp_steering_init();
    bsp_suction_init();
    app_state_machine_init(ctx);
    app_scheduler_init();
}

static int parse_input_line(const char *line, host_sil_input_t *input)
{
    unsigned long time_ms;
    int manual_arm;
    int suction_authorize;
    int transition_candidate;
    int emag_valid;
    int line_error;
    unsigned int signal_quality;
    int imu_fresh;
    int imu_id_ok;
    int pitch_cdeg;
    int encoder_valid;
    long left_count;
    long right_count;
    int left_speed_mm_s;
    int right_speed_mm_s;
    int power_ok;
    int kill_switch;
    int control_period_ok;
    int force_app_state;
    int parsed;

    imu_id_ok = 1;
    kill_switch = 0;
    control_period_ok = 1;
    force_app_state = -1;
    parsed = sscanf(line,
                    "%lu,%d,%d,%d,%d,%d,%u,%d,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%d",
                    &time_ms,
                    &manual_arm,
                    &suction_authorize,
                    &transition_candidate,
                    &emag_valid,
                    &line_error,
                    &signal_quality,
                    &imu_fresh,
                    &pitch_cdeg,
                    &encoder_valid,
                    &left_count,
                    &right_count,
                    &left_speed_mm_s,
                    &right_speed_mm_s,
                    &power_ok,
                    &imu_id_ok,
                    &kill_switch,
                    &control_period_ok,
                    &force_app_state);
    if (parsed < 15) {
        return 0;
    }

    input->time_ms = (u32)time_ms;
    input->manual_arm = manual_arm ? APP_TRUE : APP_FALSE;
    input->suction_authorize = suction_authorize ? APP_TRUE : APP_FALSE;
    input->transition_candidate = transition_candidate ? APP_TRUE : APP_FALSE;
    input->emag_valid = emag_valid ? APP_TRUE : APP_FALSE;
    input->line_error = (s16)line_error;
    input->signal_quality = (u16)signal_quality;
    input->imu_fresh = imu_fresh ? APP_TRUE : APP_FALSE;
    input->imu_id_ok = imu_id_ok ? APP_TRUE : APP_FALSE;
    input->pitch_cdeg = (s16)pitch_cdeg;
    input->encoder_valid = encoder_valid ? APP_TRUE : APP_FALSE;
    input->left_count = (s32)left_count;
    input->right_count = (s32)right_count;
    input->left_speed_mm_s = (s16)left_speed_mm_s;
    input->right_speed_mm_s = (s16)right_speed_mm_s;
    input->power_ok = power_ok ? APP_TRUE : APP_FALSE;
    input->kill_switch = kill_switch ? APP_TRUE : APP_FALSE;
    input->control_period_ok = control_period_ok ? APP_TRUE : APP_FALSE;
    input->force_app_state = (s16)force_app_state;
    return 1;
}

static void write_output_header(FILE *out)
{
    fputs("time_ms,app_state,surface_state,faults,drive_command_native,"
          "left_drive_command_native,right_drive_command_native,"
          "steering_offset_us,steering_pulse_us,suction_mode,"
          "steering_left_pulse_us,steering_right_pulse_us,"
          "logical_suction_request,hardware_suction_output,adhesion_risk,"
          "state_elapsed_ms,transition_candidate,transition_up_observed,kill_switch,"
          "track_mode,line_error_filtered,error_rate,fuzzy_kp,fuzzy_ki,fuzzy_kd,"
          "left_speed_target,right_speed_target,left_speed_measured,right_speed_measured\n",
          out);
}

static void write_output_line(FILE *out, const app_context_t *ctx, u32 now_ms)
{
    telemetry_frame_t frame;
    suction_command_t suction;

    frame = app_telemetry_make_frame(ctx, now_ms);
    suction = bsp_suction_last_command();
    fprintf(out,
            "%lu,%u,%u,%u,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            (unsigned long)frame.timestamp_ms,
            (unsigned int)frame.app_state,
            (unsigned int)frame.surface_state,
            (unsigned int)frame.faults,
            (int)frame.drive_command_native,
            (int)ctx->left_drive_command_native,
            (int)ctx->right_drive_command_native,
            (int)frame.steering_offset_us,
            (unsigned int)frame.steering_pulse_us,
            (unsigned int)suction.mode,
            (unsigned int)ctx->steering_left_pulse_us,
            (unsigned int)ctx->steering_right_pulse_us,
            (unsigned int)frame.suction_request_native,
            (unsigned int)bsp_suction_last_native_output(),
            (unsigned int)frame.adhesion_risk,
            (unsigned int)ctx->state_elapsed_ms,
            (unsigned int)ctx->transition_candidate,
            (unsigned int)(ctx->surface_state == SURFACE_TRANSITION_UP || ctx->surface_state == SURFACE_WALL),
            (unsigned int)ctx->kill_switch,
            (unsigned int)frame.track_mode,
            (int)frame.line_error_filtered,
            (int)frame.error_rate,
            (int)frame.fuzzy_kp,
            (int)frame.fuzzy_ki,
            (int)frame.fuzzy_kd,
            (int)frame.left_speed_target_mm_s,
            (int)frame.right_speed_target_mm_s,
            (int)frame.left_speed_measured_mm_s,
            (int)frame.right_speed_measured_mm_s);
}

static int run_tick_until(app_context_t *ctx, FILE *out, u32 target_time_ms)
{
    while (bsp_timebase_now_ms() < target_time_ms) {
        bsp_timebase_on_tick_1ms();
        app_scheduler_run_due(ctx, bsp_timebase_now_ms());
        write_output_line(out, ctx, bsp_timebase_now_ms());
    }
    return 1;
}

int main(int argc, char **argv)
{
    FILE *input_file;
    FILE *output_file;
    char line[512];
    host_sil_input_t input;
    app_context_t ctx;
    unsigned int line_no;

    if (argc != 3) {
        fputs("usage: host_sil <input.csv> <output.csv>\n", stderr);
        return 2;
    }

    input_file = fopen(argv[1], "r");
    if (input_file == 0) {
        fputs("failed to open input csv\n", stderr);
        return 2;
    }

    output_file = fopen(argv[2], "w");
    if (output_file == 0) {
        fclose(input_file);
        fputs("failed to open output csv\n", stderr);
        return 2;
    }

    init_system(&ctx);
    write_output_header(output_file);

    if (fgets(line, sizeof(line), input_file) == 0) {
        fclose(output_file);
        fclose(input_file);
        fputs("input csv is empty\n", stderr);
        return 2;
    }

    line_no = 1u;
    while (fgets(line, sizeof(line), input_file) != 0) {
        line_no++;
        if (line[0] == '\n' || line[0] == '\r' || line[0] == 0) {
            continue;
        }
        if (!parse_input_line(line, &input)) {
            fclose(output_file);
            fclose(input_file);
            fprintf(stderr, "failed to parse input csv line %u\n", line_no);
            return 2;
        }
        host_bsp_set_input(&input);
        run_tick_until(&ctx, output_file, input.time_ms);
    }

    fclose(output_file);
    fclose(input_file);
    return 0;
}
