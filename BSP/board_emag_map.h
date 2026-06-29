#ifndef BOARD_EMAG_MAP_H
#define BOARD_EMAG_MAP_H

#include "../App/app_types.h"

/*
 * Physical U9 ADC mapping is intentionally not verified yet.
 * Control code consumes logical A-E only; this file is the only place where
 * future ADC channel order may be bound to EMAG_A_LEFT..EMAG_E_RIGHT.
 */
/* Provisional U9 order. Must be confirmed on the real board before racing. */
#define BOARD_EMAG_A_ADC_CHANNEL 0u
#define BOARD_EMAG_B_ADC_CHANNEL 1u
#define BOARD_EMAG_C_ADC_CHANNEL 2u
#define BOARD_EMAG_D_ADC_CHANNEL 3u
#define BOARD_EMAG_E_ADC_CHANNEL 4u
#define BOARD_EMAG_LINE_ORDER_VERIFIED 0

#define BOARD_EMAG_GAIN_SCALE 1000l

#define BOARD_EMAG_A_OFFSET 0
#define BOARD_EMAG_B_OFFSET 0
#define BOARD_EMAG_C_OFFSET 0
#define BOARD_EMAG_D_OFFSET 0
#define BOARD_EMAG_E_OFFSET 0

#define BOARD_EMAG_A_GAIN 1000l
#define BOARD_EMAG_B_GAIN 1000l
#define BOARD_EMAG_C_GAIN 1000l
#define BOARD_EMAG_D_GAIN 1000l
#define BOARD_EMAG_E_GAIN 1000l

#define BOARD_EMAG_A_INVERT 0
#define BOARD_EMAG_B_INVERT 0
#define BOARD_EMAG_C_INVERT 0
#define BOARD_EMAG_D_INVERT 0
#define BOARD_EMAG_E_INVERT 0

#endif
