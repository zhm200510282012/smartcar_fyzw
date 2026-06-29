#ifndef CTRL_FUZZY_PID_H
#define CTRL_FUZZY_PID_H

#include "../App/app_types.h"
#include "../App/app_config.h"

#define FUZZY_TERM_COUNT 5u
#define FUZZY_WEIGHT_SCALE 1000

typedef enum {
    FUZZY_NB = 0,
    FUZZY_NS,
    FUZZY_ZO,
    FUZZY_PS,
    FUZZY_PB
} fuzzy_term_t;

typedef struct {
    s16 term_a;
    s16 weight_a;
    s16 term_b;
    s16 weight_b;
} fuzzy_membership_t;

typedef struct {
    s16 base_kp;
    s16 base_ki;
    s16 base_kd;
    s16 kp_adjust_limit;
    s16 ki_adjust_limit;
    s16 kd_adjust_limit;
} fuzzy_pid_base_t;

typedef struct {
    s16 dkp;
    s16 dki;
    s16 dkd;
} fuzzy_pid_adjust_t;

typedef struct {
    s16 kp;
    s16 ki;
    s16 kd;
} fuzzy_pid_gain_t;

s16 ctrl_fuzzy_pid_normalize(s16 value, s16 scale);
void ctrl_fuzzy_pid_membership(s16 normalized, fuzzy_membership_t *out);
void ctrl_fuzzy_pid_eval(s16 e_norm,
                         s16 de_norm,
                         const fuzzy_pid_base_t *base,
                         fuzzy_pid_gain_t *gain,
                         fuzzy_pid_adjust_t *adjust);
void ctrl_fuzzy_pid_rule_value(u8 e_term,
                               u8 de_term,
                               const fuzzy_pid_base_t *base,
                               fuzzy_pid_adjust_t *adjust);

#endif
