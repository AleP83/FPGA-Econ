/**
 * @file hw.h
 * @brief A Documented file of functions used in fpga code.
 */
#pragma once

#include "../common/definitions.h"
#if USE_HLS_LIB
// #include <hls_math.h>
#else
#include <math.h>
#endif

#include <iostream>
#include <ctime>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/** Structs */
typedef struct hw_env_t
{
	real alpha;			///< share of the capital in production {0-1}

} hw_env_t;

extern "C"
{
	/**
	 * @brief initialize
	 *
	 * @param st_k			initialize array
	 */
	void hw_top_init(real *k_in,fixed_t st_k[J]);

	void hw_loop(fixed_t st_k[J],
				 fixed_t &reduced_sum);
	
	void runOnfpga(preinit_t *preinit,
				   out_t *out);
} // end extern C

#if(J == 8 && FIXED_PRECISION)
static const fixed_t fxd_km_grid[NKM_GRID] = {
	30.000000000000000,
	36.666666666666664,
	43.333333333333336,
	50.000000000000000};
#endif
