#pragma once
#include <stdio.h>
#include <stdlib.h> // must be prior to sds_lib.h
#include <string.h>
#include <assert.h>
// #include <cstdint>

/** Development options */
#include "dev_options.h"

/** Hardware includes */
typedef double real;
#if FIXED_ACC
#include <ap_int.h>
#include <ap_fixed.h>
typedef ap_fixed<72, 21> fixed_t;	// ap_[u]fixed<W,I,Q,O,N> where W is word length in bits and I is no. of bits above the decimal point
#else
typedef real fixed_t;	
#endif
#define ID_MASTER           0       ///< never touch it
#define N_MODEL             1       ///< total number of models
/** Application constants */
#define J 8 
const real KMIN(0.1);				// minimum grid-value of array
const real KMAX(1000.0);			// maximum grid-value of array
const real power_polynomial_KGRID(7.);	// Exponent of polynomial rule on capital grid

/** Paths */
#define MAX_FILENAME_LEN 150
#define FPGA_REDUCE_OUT_FILE	"./results/fpga/final_values/sum_"
#define CPU_REDUCE_OUT_FILE		"./results/cpu/final_values/sum_"
#define CPU_EXEC_OUT_FILE		"./results/cpu/exec_time_"
#define FPGA_EXEC_OUT_FILE		"./results/fpga/exec_time_"

/** Parameters and constant arrays */
typedef struct env_t
{
	real k[J];								// grid of capital
} env_t;
/**
 * @brief final results to be read back
 *
 */
typedef struct out_t
{
	real reduced_sum;
} out_t;
/**
 * @brief pre initialized values to device
 *
 */
typedef struct preinit_t
{
	real k[J];
} preinit_t;



/** Typedefs */
/*
typedef double real;
typedef unsigned short small_idx_t;
typedef unsigned int idx_t;
typedef unsigned char shock_t; 
#if FIXED_ACC
typedef ap_fixed<24,16> fixed_t;	// ap_[u]fixed<W,I,Q,O,N> where W is word length in bits and I is no. of bits above the decimal point
#else
typedef real fixed_t;	
#endif
*/