#ifndef APP_PARAMS_H
#define APP_PARAMS_H

#include "app_types.h"

typedef struct {
    const char *name;
    s32 value;
    const char *unit;
    const char *range_note;
    const char *status;
    const char *risk;
} app_param_entry_t;

const app_param_entry_t *app_params_table(u16 *count);

#endif
