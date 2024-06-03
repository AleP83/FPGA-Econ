#include "hw.h"
extern "C"
{
	/**
	 * First-time setup as separate top-level function
	 * Copy objects from Global to Local Memory (BRAM/URAM)
	 */
	void hw_top_init(real *k_in, fixed_t st_k[J]){
		init_1:
		for (int i = 0; i < J; ++i){
			st_k[i] = (fixed_t) k_in[i];
		}
		return;
	}

	void runOnfpga(
		preinit_t *preinit,
		out_t *out)
	{
		// Local variables
		fixed_t st_k[J];
		
		// Copy data global memory to on-chip memory (BRAM, URAM) - burst transfer
		hw_top_init(preinit->k,st_k);

		// Estimation Loop
		fixed_t reduced_sum= 0.;	  
		hw_loop(st_k,reduced_sum);
		out->reduced_sum = (real) reduced_sum;
		
		return;
	}

	void hw_loop(fixed_t st_k[J],fixed_t &reduced_sum){
		#pragma HLS array_partition variable = st_k factor = 4 type = cyclic	
    	fixed_t sum = 0.;
		loop_reduce:
		for(int i=0;i<J;i++) {
			#pragma HLS unroll factor=8
			#pragma HLS pipeline II=1
		    
            sum+=st_k[i];
        }
		reduced_sum = sum;
		return;
	}
}
