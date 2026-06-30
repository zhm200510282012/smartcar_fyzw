#ifndef BOARD_EMAG_MAP_H
#define BOARD_EMAG_MAP_H

#include "../App/app_types.h"

/*
 * Mapping-layer alignment from last year's hardware resource order only.
 * Logical A-E are fixed from left to right at the car nose:
 * A=L1, B=L2, C=M, D=R1, E=R2.
 *
 * Last year's Inductor.c reads:
 * L1/A=ADC5, L2/B=ADC4, M/C=ADC3, R1/D=ADC0, R2/E=ADC1.
 * This is not a bench-verified line-order claim yet.
 */
#define BOARD_EMAG_A_ADC_CHANNEL 5u
#define BOARD_EMAG_B_ADC_CHANNEL 4u
#define BOARD_EMAG_C_ADC_CHANNEL 3u
#define BOARD_EMAG_D_ADC_CHANNEL 0u
#define BOARD_EMAG_E_ADC_CHANNEL 1u
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
