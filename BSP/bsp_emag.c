#include "bsp_emag.h"
#include "board_emag_map.h"

void bsp_emag_init(void)
{
}

emag_sample_t bsp_emag_read(void)
{
    emag_sample_t s;
    u8 i;
    for (i = 0u; i < EMAG_CHANNEL_COUNT; i++) {
        s.raw[i] = 0u;
        s.filtered[i] = 0u;
        s.norm[i] = 0u;
    }
    s.line_error = 0;
    s.line_quality = 0u;
    s.signal_quality = 0u;
    s.channel_count = 0u;
    s.line_lost = APP_TRUE;
    s.valid = APP_FALSE;
    return s;
}
