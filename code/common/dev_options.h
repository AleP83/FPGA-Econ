/**
 * Platform / input options
 */

/**
 * Debug acceleration options
 */

// Select FPGA design by enabling exaclty one of the following macros, keeping the rest to zero. For best performance, set _ACROSS_ECONOMY to 1 and rest 0
#define _BASELINE 0                     // FPGA design with no HLS acceleration.
#define _PIPELINE 0                     // FPGA design with only PIPELINE acceleration
#define _WITHIN_ECONOMY 0               // FPGA design with one-kernel data parallelization and pipelining
#define _ACROSS_ECONOMY 1               // FPGA design with three-kernels. Benchmark

// The following 3 algorithms are explored in the CPU implementation.  
// Select only one of the following macros to 1, keeping the rest to zero. For best performance, set _CUSTOM_BINARY_SEARCH to 1 and rest 0
#define _LINEAR_SEARCH 0
#define _BINARY_SEARCH 0
#define _CUSTOM_BINARY_SEARCH 1

#ifdef _FPGA_MODE
#if _BASELINE
#define NUM_KERNELS 1         // N. Kernels: more than 1 only used for fpga
#define FIXED_ACC 0           // set this to use fixed point for accumulation in egm and ast
#define KCROSS_PIP_OFF 1      // turn off pipelining of cross-sectional distribution of capital holdings
#define PARTITION_KCROSS 1    // partition of cross-sectional distribution of capital holdings with cyclic factor
#define AST_UNROLL 0          // set this to 1 for unrolling ast loop
#define NUM_KPRIMES 1         // set the number of kprime copies to be created
#define SMALL_PL 1            // turn off ihp pipelining optimization
#define IHP2_UNROLL 0         //
#define SPREAD_SCALAR_TYPE 0  // Data type of spread  
#elif _PIPELINE
#define NUM_KERNELS 1         // N. Kernels: more than 1 only used for fpga
#define FIXED_ACC 1           // set this to use fixed point for accumulation in egm and ast
#define KCROSS_PIP_OFF 0      // turn off pipelining of cross-sectional distribution of capital holdings
#define PARTITION_KCROSS 1    // partition of cross-sectional distribution of capital holdings with cyclic factor
#define AST_UNROLL 0          // set this to 1 for unrolling ast loop
#define NUM_KPRIMES 4         // set the number of kprime copies to be created
#define SMALL_PL 0            //
#define IHP2_UNROLL 0         //
#define SPREAD_SCALAR_TYPE 1  // Data type of spread 
#elif _WITHIN_ECONOMY
#define NUM_KERNELS 1         // N. Kernels: more than 1 only used for fpga
#define FIXED_ACC 1           // set this to use fixed point for accumulation in egm and ast
#define KCROSS_PIP_OFF 0      // turn off pipelining of cross-sectional distribution of capital holdings
#define PARTITION_KCROSS 8    // partition of cross-sectional distribution of capital holdings with cyclic factor
#define AST_UNROLL 1          // set this to 1 for unrolling ast loop
#define NUM_KPRIMES 8         // set the number of kprime copies to be created
#define SMALL_PL 0            //
#define IHP2_UNROLL 1         //
#define SPREAD_SCALAR_TYPE 1  // Data type of spread
#elif _ACROSS_ECONOMY
#define NUM_KERNELS 3         // N. Kernels: more than 1 only used for fpga
#define FIXED_ACC 1           // set this to use fixed point for accumulation in egm and ast
#define KCROSS_PIP_OFF 0      // turn off pipelining of cross-sectional distribution of capital holdings
#define PARTITION_KCROSS 8    // partition of cross-sectional distribution of capital holdings with cyclic factor
#define AST_UNROLL 1          // set this to 1 for unrolling ast loop
#define NUM_KPRIMES 8         // set the number of kprime copies to be created
#define SMALL_PL 0            //
#define IHP2_UNROLL 0         //
#define SPREAD_SCALAR_TYPE 1  // Data type of spread
#endif
#elif  defined(_SERIAL_CPU_MODE) || defined(_OPENMPI_MODE)
#define NUM_KERNELS 1         // N. Kernels: more than 1 only used for fpga
#define FIXED_ACC 0           // set this to use fixed point for accumulation in egm and ast
#define KCROSS_PIP_OFF 0      // turn off pipelining of cross-sectional distribution of capital holdings
#define PARTITION_KCROSS 1    // partition of cross-sectional distribution of capital holdings with cyclic factor
#define AST_UNROLL 0          // set this to 1 for unrolling ast loop
#define NUM_KPRIMES 1         // set the number of kprime copies to be created
#define SMALL_PL 1            // turn off ihp pipelining optimization
#define IHP2_UNROLL 0         //
#define SPREAD_SCALAR_TYPE 0  // Data type of spread
#endif

#define DETAILED_PERF_METRICS 1 // set to print detailed timing values
#define SHOCKS_FROM_HEADER 1    // Use shocks from consh.h file

/**
 * Usage of HLS library in sw_emu
 */
#define USE_HLS_LIB 0

/**
 * Experiments
 *	Move to another section when mature
 */
#define IDS_AGG_PACK			0   // If 1, AST_UNROLL must be 1?
#if IDS_AGG_PACK
#define IDS_AGG_X				1   // bytes packed into shock_t
#else
#define IDS_AGG_X 1
#endif
#define NUM_KCROSS				(8 * IDS_AGG_X)
#ifndef _FPGA_MODE
#define PACK_IDS				0
#else
#define PACK_IDS				1       //  set this to pack 8 bytes into each row to be stored in URAM
#endif
#define NEW_IDSHOCK_SIZE 		172700  // ceil((10000/8)*(1/8))

/**
 * Paths for storing the results
 */
#define KP_OUT_FILE "./results/fpga/final_values/kpo_"
#define KCROSS_OUT_FILE "./results/fpga/final_values/kcross_"
#define COEFFS_OUT_FILE "./results/fpga/final_values/coeffs_"
#define R2BG_OUT_FILE "./results/fpga/final_values/r2bg_"
#define ITER_OUT_FILE "./results/fpga/final_values/egm_iter_"

#define CPU_KP_OUT_FILE "./results/cpu/final_values/kpo_"
#define CPU_KCROSS_OUT_FILE "./results/cpu/final_values/kcross_"
#define CPU_COEFFS_OUT_FILE "./results/cpu/final_values/coeffs_"
#define CPU_R2BG_OUT_FILE "./results/cpu/final_values/r2bg_"
#define CPU_ITER_OUT_FILE "./results/cpu/final_values/egm_iter_"

#define OPENMPI_KP_OUT_FILE "./results/cpu/final_values/kpo_"
#define OPENMPI_KCROSS_OUT_FILE "./results/cpu/final_values/kcross_"
#define OPENMPI_COEFFS_OUT_FILE "./results/cpu/final_values/coeffs_"
#define OPENMPI_R2BG_OUT_FILE "./results/cpu/final_values/r2bg_"
#define OPENMPI_ITER_OUT_FILE "./results/cpu/final_values/egm_iter_"
