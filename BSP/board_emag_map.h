#ifndef BOARD_EMAG_MAP_H
#define BOARD_EMAG_MAP_H

#include "../App/app_types.h"

/*
 * Physical U9 ADC mapping is intentionally not verified yet.
 * Control code consumes logical A-E only; this file is the only place where
 * future ADC channel order may be bound to EMAG_A_LEFT..EMAG_E_RIGHT.
 */
#define BOARD_EMAG_A_ADC_INDEX 0u
#define BOARD_EMAG_B_ADC_INDEX 1u
#define BOARD_EMAG_C_ADC_INDEX 2u
#define BOARD_EMAG_D_ADC_INDEX 3u
#define BOARD_EMAG_E_ADC_INDEX 4u
#define BOARD_EMAG_LINE_ORDER_VERIFIED 0

#endif
