/**
 * Debug acceleration options
 */
#define DETAILED_PERF_METRICS	1 // set to print detailed timing values
#ifdef _FPGA_MODE
#define FIXED_ACC				1   // set this to use fixed point precision
#define NUM_KERNELS             1   // more than 1 only used for fpga 
#else
#define FIXED_ACC				0   // set this to use fixed point precision
#define NUM_KERNELS             1   // more than 1 only used for fpga 
#endif

/**
 * Usage of HLS library in sw_emu
 */
#define USE_HLS_LIB				0
