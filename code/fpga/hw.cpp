#include "hw.h"
#ifndef __SYNTHESIS__
#define PRINT_LOOP_CNT 1
#endif

extern "C"
{
	/** Constants moved to hw.h*/

	/** Static on-PL memories */
	static hw_env_t st_env;

	/**
	 * First-time setup as separate top-level function
	 */
	void hw_top_init(
		real st_kprimes[NUM_KPRIMES][NSTATES], real st_kcross[N_AGENTS]
#ifndef _FPGA_MODE
		, preinit_t preinit
#endif
	)
	{
	init_1:
		for (int j = 0; j < NSTATES; ++j)
		{
#ifdef _FPGA_MODE
			real val = kp_in[j];
#else
			real val = preinit.kprime[j];
#endif
			for (int k = 0; k < NUM_KPRIMES; ++k)
			{
				st_kprimes[k][j] = val;
			}
		}

	init_2:
		for (int j = 0; j < N_AGENTS; ++j)
		{
			st_kcross[j] = env__kss;
		}

	init_3:
		for (int i = 0; i < NKM_GRID; ++i)
		{
#ifndef _FPGA_MODE
			st_env.km[i] = preinit.km[i];
#endif
		}

	init_4:
		for (int i = 0; i < NKGRID; ++i)
		{
#ifndef _FPGA_MODE
			st_env.k[i] = preinit.k[i];
#endif
		}

	init_5:
		for (int i = 0; i < NSTATES; ++i)
#ifndef _FPGA_MODE
			st_env.wealth[i] = preinit.wealth[i];
#endif

		hw_init_env();

		st_env.irate_factor[0] = 0.356400000000000;
		st_env.irate_factor[1] = 0.363600000000000;

		st_env.wage_factor[0] = 0.633600000000000;
		st_env.wage_factor[1] = 0.646400000000000;

		st_env.cons2_factor[0] = 0.150000000000000;
		st_env.cons2_factor[1] = 1.094444444444445;
		st_env.cons2_factor[2] = 0.150000000000000;
		st_env.cons2_factor[3] = 1.104861111111111;

#if (NKM_GRID == 4)
		st_env.log_env_km[0] = 3.401197433471680;
		st_env.log_env_km[1] = 3.601868152618408;
		st_env.log_env_km[2] = 3.768922090530396;
		st_env.log_env_km[3] = 3.912023067474365;
#elif (NKM_GRID == 8)
		st_env.log_env_km[0] = 3.401197433471680;
		st_env.log_env_km[1] = 3.492169141769409;
		st_env.log_env_km[2] = 3.575550794601440;
		st_env.log_env_km[3] = 3.652511835098267;
		st_env.log_env_km[4] = 3.723970890045166;
		st_env.log_env_km[5] = 3.790662050247192;
		st_env.log_env_km[6] = 3.853182554244995;
		st_env.log_env_km[7] = 3.912023067474365;
#endif

		return;
	}

	void runOnfpga(
		const unsigned char *hw_agshock,
		const unsigned char *hw_idshock,
		preinit_t *preinit,
		out_t *results,
		int *hw_iter)
	{
		#pragma HLS INTERFACE m_axi port=hw_agshock offset=slave bundle=gmem0
		#pragma HLS INTERFACE m_axi port=hw_idshock offset=slave bundle=gmem0
		#pragma HLS INTERFACE m_axi port=preinit offset=slave bundle=gmem0
		#pragma HLS INTERFACE m_axi port=results offset=slave bundle=gmem0
		#pragma HLS INTERFACE m_axi port=hw_iter offset=slave bundle=gmem0

		unsigned char agshock[AGSHOCK_ARR_SIZE];
#if PACK_IDS
		ap_uint<72> idshock[NEW_IDSHOCK_SIZE] = {0};
#else
		unsigned char idshock[IDSHOCK_ARR_SIZE] = {0};
#endif

		real st_kcross[N_AGENTS];
		real st_kprimes[NUM_KPRIMES][NSTATES];
		real kmts[SIM_STEPS];
		real r2[NSTATES_AG];
		real kmprime[NSTATES_AG * NKM_GRID];
		real coeff[NCOEFF] = {0, 1, 0, 1};
		real metric_coeff = 1000; // some large number

#if PACK_IDS
#pragma HLS bind_storage variable = idshock type = RAM_1P impl = URAM
#endif
#pragma HLS array_partition variable = kmprime complete
#pragma HLS array_partition variable = k_grid complete dim = 1
#pragma HLS array_partition variable = km_grid complete dim = 1

	// burst transfer the data from global memory to on-chip memory (BRAM, URAM)
	main_1:
		for (int i = 0; i < AGSHOCK_ARR_SIZE; i++)
		{
			agshock[i] = hw_agshock[i];
		}

#if PACK_IDS
	// use URAM to store the idshocks
	// 8 idshocks are packed into 1 byte-> (1,100 * 10,000 / 8) = 1,375,000 bytes
	// copy to data variable of size 64 bits. Hence, 8 input bytes are copied to one element
	main_2: // loop over each of the 1,100 time step. (10,000 / 8) = 1250
		for (int i = 0, j = 0; i < IDSHOCK_ARR_SIZE; i = i + 1250)
		{
#pragma HLS pipeline off
		main_2_2: // for each time step, copy 8 bytes into a single element of size 64 bits
			for (int k = 0; k < 1250; j++)
			{
				// handle edge case where last 2 bytes are remaining since 1,250 is not devisible by 8
				if (k == 1248)
				{
					idshock[j] = (hw_idshock[i + k + 1] << 8) | (hw_idshock[i + k]);
					k = k + 2;
				}
				else
				{
					idshock[j] = (((ap_uint<72>)hw_idshock[i + k + 7] << 56) |
								  ((ap_uint<72>)hw_idshock[i + k + 6] << 48) |
								  ((ap_uint<72>)hw_idshock[i + k + 5] << 40) |
								  ((ap_uint<72>)hw_idshock[i + k + 4] << 32) |
								  ((ap_uint<72>)hw_idshock[i + k + 3] << 24) |
								  ((ap_uint<72>)hw_idshock[i + k + 2] << 16) |
								  ((ap_uint<72>)hw_idshock[i + k + 1] << 8) |
								  ((ap_uint<72>)hw_idshock[i + k + 0]));
					k = k + 8;
				}
			}
		}

#else
	// use BRAM to store the idshocks
	main_2:
		for (int i = 0; i < IDSHOCK_ARR_SIZE; i++)
		{
			idshock[i] = hw_idshock[i];
		}
#endif

		hw_top_init(st_kprimes, st_kcross
#ifndef _FPGA_MODE
		, *preinit
#endif
		);

#if PRINT_LOOP_CNT
		int iter_main = 0;
#endif

		int hw_main_iter = 0;		// total number of ihp calls
		int curr_ihp_iter = 0;		// number of ihp iterations in each ihp call
		int hw_ihp_iter[300] = {0}; // local mem array to store the number of ihp iterations

#ifndef __SYNTHESIS__
		printf("metric_coeff = %lf\n", metric_coeff);
#endif

		while (metric_coeff > TOLL_COEFF)
		{
			hw_main_iter++;
			hw_sim_alm(kmprime, coeff); // step 1

			curr_ihp_iter = 0;
			hw_sim_ihp(st_kprimes, kmprime, curr_ihp_iter); // step 2
			hw_ihp_iter[hw_main_iter] = curr_ihp_iter;		// start from 1st element of hw_ihp_iter

			real kcross_l[N_AGENTS];
			kc_t kcross_mean = 0;

		ast_kcross:
			for (int is = 0; is < N_AGENTS; is++)
			{
#if KCROSS_PIP_OFF 				
				#pragma HLS pipeline off
#endif
				kcross_l[is] = st_kcross[is];
				kcross_mean += (kc_t)st_kcross[is];
			}

			hw_sim_ast(kmts, st_kprimes, kcross_l, agshock, idshock, kcross_mean); // step 3

			sim_alm_coeff(kmts, coeff, &metric_coeff, r2, agshock); // step 4

			if (metric_coeff > TOLL_COEFF * 100)
			{
				// Replace the old with new capital distribution
				for (int j = 0; j < N_AGENTS; j++)
				{
					st_kcross[j] = kcross_l[j];
				}
			}

#if PRINT_LOOP_CNT
			iter_main++;
			printf("main loop iter = %d\n", iter_main);
#endif
		}

	// burst transfer from on-chip memory to global memory
	main_5:
		for (int i = 0; i < NSTATES_AG; i++)
		{
			results->r2[i] = r2[i];
		}

	main_7:
		for (int i = 0; i < NSTATES; i++)
		{
			results->kprime[i] = st_kprimes[0][i];
		}

	main_10:
		for (int i = 0; i < N_AGENTS; i++)
		{
			results->kcross[i] = st_kcross[i];
		}

	main_11:
		for (int i = 0; i < NCOEFF; i++)
		{
			results->coeff[i] = coeff[i];
		}

		hw_iter[0] = hw_main_iter;

	main_12:
		for (int i = 1; i <= hw_iter[0]; i++)
		{
			hw_iter[i] = hw_ihp_iter[i];
		}

		return;
	}

	void hw_sim_alm(real *kmprime, real *coeff)
	{
		small_idx_t cidx = 0;
		real c0, c1;
		small_idx_t kidx = 0;

	alm_1:
		for (int ia = 0; ia < NSTATES_AG; ++ia)
		{
			c0 = coeff[cidx];
			c1 = coeff[cidx + 1];
			cidx += REGRESSORS;
		alm_2:
			for (int ikm = 0; ikm < NKM_GRID; ++ikm)
			{
#pragma HLS unroll factor = 1
				// #pragma HLS pipeline
				// #endif
				// real val = hw_exp(c0
				// 		+ c1
				// 		* st_env.log_env_km[ikm]);
#if defined(_SERIAL_CPU_MODE) || defined(_OPENMPI_MODE)
				real t_log = hw_log(st_env.km[ikm]);
#else
				real t_log = hw_log(km_grid[ikm]);
#endif 
				real t_mul = c1 * t_log;
				real t_add = c0 + t_mul;
				real val = hw_exp(t_add);			  // hw_exp(c0 + c1 * st_env.log_env_km[ikm])
				hw_rail_values(&val, KM_MAX, KM_MIN); // eq 15
				kmprime[kidx++] = val;
			}
		}
		return;
	}

	void hw_sim_ihp(real st_kprimes[NUM_KPRIMES][NSTATES], real *kmprime, int &hw_ihp_iter)
	{

#if (NUM_KPRIMES == 8)
// #if (NUM_KPRIMES == 8 && _WITHIN_ECONOMY)
#pragma HLS array_partition variable = st_env.P complete 
#pragma HLS array_partition variable = st_kprimes complete dim = 1
#pragma HLS bind_storage variable = st_kprimes type = RAM_1WNR impl = BRAM
#else
#pragma HLS array_partition variable = st_kprimes complete dim = 1
#pragma HLS bind_storage variable = st_kprimes type = RAM_2P impl = BRAM
#endif

#if AST_UNROLL
#pragma HLS array_partition variable = st_env.epsilon2 complete dim = 1
#endif

		/** Lookup tables */
		// substitute for IXV call
		static const small_idx_t li_2d_aux_idx_base[4] = {
			0,
			NKGRID,
			NKM_GRID * NSTATES_ID * NKGRID,
			(NKGRID + NKM_GRID * NSTATES_ID * NKGRID)};

#pragma HLS array_partition variable = li_2d_aux_idx_base complete

		real kprime_new[NSTATES]; // Local kprime/new copies
		real metric = 1;		  //
#if PRINT_LOOP_CNT
		int iter_cnt = 0;
#endif

	// Convergence loop: 4 x NSTATES interp over kprime[]
	ihp_1:
		while (metric > (real)TOLL_K) // eq 14
		{
			hw_ihp_iter++;
#pragma HLS loop_tripcount min = 1 avg = 200 max = 2000

#if (IHP2_UNROLL==1)
			spread_t global_spread_scalar = VERY_SMALL_SCALAR;
			spread_t spread_scalar[2] = {VERY_SMALL_SCALAR, VERY_SMALL_SCALAR};
#else
			spread_t spread_scalar = VERY_SMALL_SCALAR;
#endif

			// Reset index values for [1600] loop
			small_idx_t p_idx_outer = 0b0100; // 4
			small_idx_t hundreds_cnt = NKGRID;
			small_idx_t kp_iter_cnt = (NSTATES_ID * NKGRID);
			small_idx_t kidx = 0;
		ihp_2:
			for (small_idx_t is = 0; is < NSTATES; ++is)
			{
#if SMALL_PL
#pragma HLS unroll factor = 1
#elif (IHP2_UNROLL==1)
#pragma HLS unroll factor = 2
#pragma HLS pipeline
#else
#pragma HLS pipeline
#endif

				small_idx_t p_idx_inner = 0; // IIDP x IAP
				real kmp, temp_base;
				emu_s_t emu_s = 0.;
				real kp = st_kprimes[0][is];

				// Index handling
				if (++kp_iter_cnt >= NSTATES_ID * NKGRID)
				{
					kp_iter_cnt = 0;
					kmp = kmprime[kidx++];
					temp_base = kmp * (real)env__l_bar_inv;
				}
				if (++hundreds_cnt >= NKGRID)
				{
					hundreds_cnt = 0;
					//(changes between 0 and 4 for every 100 iterations uptil is = 800,
					// and changes between 8 and 12 for every 100 iterations uptil is = 1600)
					p_idx_outer ^= (small_idx_t)0b0100; // (XOR at every bit) 0100 ^ 0100 = 0000 -> 0 (explicit conversion to short) decimal value
				}
				if (is == (NKM_GRID * NSTATES_ID * NKGRID)) // 800 ia
					p_idx_outer |= (small_idx_t)0b1000;			// (OR at every bit) 0000 | 1000 = 1000 -> 8 (explicit conversion to short) decimal value
				p_idx_inner = 0;							// not necessary? check it
			// 4x ihp 2D interpolation over kprime[]
			ihp_3:
				for (int iap = 0; iap < NSTATES_AG; ++iap)
				{
#if SMALL_PL
#pragma HLS unroll factor = 1
#endif
					real temp = temp_base * st_env.er_inv[iap];
					real temp_irate = pow(temp, env__alpha_c);
					real irate = st_env.irate_factor[iap] * temp_irate;
					#pragma HLS bind_op variable=irate op=fmul
					real imrt = env__delta_c + irate;
					real temp_wage = pow(temp, env__alpha);
					real wage = st_env.wage_factor[iap] * temp_wage;
					#pragma HLS bind_op variable=wage op=fmul
					small_idx_t kpb = iap << 2;
				ihp_4:
					for (int iidp = 0; iidp < NSTATES_ID; ++iidp)
					{
#if SMALL_PL
#pragma HLS pipeline
#endif
#if defined(_SERIAL_CPU_MODE) || defined(_OPENMPI_MODE)
						small_idx_t i1_min = hw_findrange(kmp, st_env.km, NKM_GRID);
						small_idx_t i1_max = i1_min + 1;
						real i1_min_val = st_env.km[i1_min];
						real i1_max_val = st_env.km[i1_max];
						small_idx_t i2_min = hw_findrange(kp, st_env.k, NKGRID);
						small_idx_t i2_max = i2_min + 1;
						real i2_min_val = st_env.k[i2_min];
						real i2_max_val = st_env.k[i2_max];
#else
						small_idx_t i1_min = hw_findrange((fixed_t)kmp, fxd_km_grid, NKM_GRID);
						small_idx_t i1_max = i1_min + 1;
						real i1_min_val = km_grid[i1_min];
						real i1_max_val = km_grid[i1_max];
						small_idx_t i2_min = hw_findrange((fixed_t)kp, fxd_k_grid, NKGRID);
						small_idx_t i2_max = i2_min + 1;
						real i2_min_val = k_grid[i2_min];
						real i2_max_val = k_grid[i2_max];
#endif					
						small_idx_t idx_base = li_2d_aux_idx_base[p_idx_inner];
						small_idx_t i1_min_base = idx_base + (NSTATES_ID * NKGRID * i1_min);
						small_idx_t i1_max_base = idx_base + (NSTATES_ID * NKGRID * i1_max);
						real tz_num = (kmp - i1_min_val);
						real tz_den = (i1_max_val - i1_min_val);
						real tz = tz_num / tz_den;
						real tw_num = (kp - i2_min_val);
						real tw_den = (i2_max_val - i2_min_val);
						real tw = tw_num / tw_den;
						real sub_tz = (1.0 - tz);
						real sub_tw = (1.0 - tw);
						real sub_tz_sub_tw = sub_tz * sub_tw;
						real tz_tw = tz * tw;
						real sub_tz_tw = sub_tz * tw;
						real tz_sub_tw = tz * sub_tw;
#if (NUM_KPRIMES == 1)
						real fp_1 = st_kprimes[0][i1_min_base + i2_min] * sub_tz_sub_tw;
						real fp_2 =	st_kprimes[0][i1_min_base + i2_max] * sub_tz_tw;
						real fp_3 = st_kprimes[0][i1_max_base + i2_min] * tz_sub_tw;
						real fp_4 = st_kprimes[0][i1_max_base + i2_max] * tz_tw;
#elif (NUM_KPRIMES == 4)
						real fp_1 = st_kprimes[0][i1_min_base + i2_min] * sub_tz_sub_tw;
						real fp_2 =	st_kprimes[1][i1_min_base + i2_max] * sub_tz_tw;
						real fp_3 = st_kprimes[2][i1_max_base + i2_min] * tz_sub_tw;
						real fp_4 = st_kprimes[3][i1_max_base + i2_max] * tz_tw;
#elif (NUM_KPRIMES == 8)
						real fp_1 = st_kprimes[kpb + 0][i1_min_base + i2_min] * sub_tz_sub_tw;
						real fp_2 =	st_kprimes[kpb + 1][i1_min_base + i2_max] * sub_tz_tw;
						real fp_3 = st_kprimes[kpb + 2][i1_max_base + i2_min] * tz_sub_tw;
						real fp_4 = st_kprimes[kpb + 3][i1_max_base + i2_max] * tz_tw;
#endif
						real fp_5 = fp_1 + fp_2;
						real fp_6 = fp_3 + fp_4;
						real fp = fp_5 + fp_6;
						real cons2_1 = imrt * kp;
						real cons2_2 = wage * st_env.cons2_factor[p_idx_inner];
						real cons2_temp = (cons2_1 + cons2_2);
						real cons2 = cons2_temp - fp;
						if (cons2 < 0) //  eq 11
							cons2 = CONS2_MIN;
						real mu2 = (1.0/cons2);
						// real mu2 = hw_pow(cons2, env__gamma_neg);
						real emu_s_1 = imrt * mu2;
						emu_s += (emu_s_t)(st_env.P[p_idx_outer + p_idx_inner] * emu_s_1);
						++p_idx_inner;
					}
				} // end 4x ihp interpolation over kprime[]
// ~ Today (each 4x interpolate yields one new kprime value)
#ifdef _FPGA_MODE
				real temp1_new_kp = (env__beta * (real)emu_s);
				real temp2_new_kp = (1.0/temp1_new_kp);
				real new_kp = init_wealth[is] - temp2_new_kp; //  eq 10
				// real new_kp = init_wealth[is] - hw_pow(env__beta * (real)emu_s, env__gamma_neg_inv); //  eq 10
#else
				real new_kp = st_env.wealth[is] - hw_pow(env__beta * (real)emu_s, env__gamma_neg_inv);
#endif
				hw_rail_values(&new_kp, KMAX, KMIN);
				// ~ Convergence check

				spread_t spread = (spread_t)hw_fabs(new_kp - kp);
#if (IHP2_UNROLL==1)
				if (spread > spread_scalar[is%2])
					spread_scalar[is%2] = spread;
#else
				if (spread > spread_scalar)
					spread_scalar = spread;
#endif
				kprime_new[is] = new_kp;
			} //end of ihp_2

		// ~Update guess: new kprimes[] for next top-level call
		ihp_5:
			for (small_idx_t is = 0; is < NSTATES; ++is)
			{
#pragma HLS pipeline
				real temp1_kp = UPDATE_K * kprime_new[is]; // eq 13
				real temp2_kp = UPDATE_K_C * st_kprimes[0][is]; // eq 13
				real updated_kp = temp1_kp + temp2_kp; // eq 13
				for (small_idx_t k = 0; k < NUM_KPRIMES; ++k)
					st_kprimes[k][is] = updated_kp;
			}

#if (IHP2_UNROLL==1)
			global_spread_scalar = (spread_scalar[0]<=spread_scalar[1]) ? spread_scalar[1] : spread_scalar[0];
			spread_scalar[0] = global_spread_scalar;
			spread_scalar[1] = global_spread_scalar;
			// ~ Update metric
			metric = (real)global_spread_scalar;
#else
			// ~ Update metric
			metric = (real)spread_scalar;
#endif

#if PRINT_LOOP_CNT
			++iter_cnt;
			if (iter_cnt % 100 == 0) // print every 100 iterations
				printf("ihp iter count = %d, spread scalar = %.15lf\n", iter_cnt, metric);
#endif
		}
#if PRINT_LOOP_CNT
		printf("ihp iter count = %d, spread scalar = %.15lf\n", iter_cnt, metric);
#endif

		return;
	}

	void hw_sim_ast(real *kmts,
					real st_kprimes[NUM_KPRIMES][NSTATES],
					real st_kcross[N_AGENTS],
					unsigned char agshock[AGSHOCK_ARR_SIZE],
#if PACK_IDS
					ap_uint<72> idshock[NEW_IDSHOCK_SIZE],
#else
					shock_t idshock[NEW_IDSHOCK_SIZE],
#endif
					kc_t kcross_mean)
	{

#if (PARTITION_KCROSS == 1)
#pragma HLS array_partition variable = st_kcross type = cyclic factor = 1
#elif (PARTITION_KCROSS == 4)
#pragma HLS array_partition variable = st_kcross type = cyclic factor = 4
#elif (PARTITION_KCROSS == 8)
#pragma HLS array_partition variable = st_kcross type = cyclic factor = 8
#endif

#pragma HLS bind_storage variable = st_kcross type = RAM_S2P impl = BRAM

#if AST_UNROLL
		real kprime_interp0[NUM_KCROSS][NSTATES_ID * NKGRID];
		real kprime_interp1[NUM_KCROSS][NSTATES_ID * NKGRID];
		#pragma HLS array_partition variable = kprime_interp0 complete dim = 1
		#pragma HLS array_partition variable = kprime_interp1 complete dim = 1
		#pragma HLS array_partition variable = st_env.epsilon2 complete dim = 1
#else
		real kprime_interp0[NSTATES_ID * NKGRID];
		real kprime_interp1[NSTATES_ID * NKGRID];
#endif

#if PACK_IDS
		small_idx_t idshock_cnt = 64;
		ap_uint<72> temp_ids = idshock[0];
#else
		small_idx_t idshock_cnt = 8;
#endif
		int idshock_idx = 0;
		idx_t agshock_idx = 0;
		shock_t curr_ids;
		shock_t curr_ags;
		small_idx_t ags_phase = AGS_PACK_FACTOR;

// Loop 1
#if PRINT_LOOP_CNT
		int iter_cnt = 0;
#endif

		real curr_kmts = (real)kcross_mean * N_AGENTS_INV;
		hw_rail_values(&curr_kmts, KM_MAX, KM_MIN);

	ast_1:
		for (int t = 0; t < SIM_STEPS; ++t)
		{
#if PRINT_LOOP_CNT
			++iter_cnt;
#endif

			kmts[t] = curr_kmts;

			// Read next packed agshock value when needed
			if (++ags_phase >= AGS_PACK_FACTOR)
			{
				curr_ags = agshock[agshock_idx++];
				ags_phase = 0;
			}

			bool p0 = (curr_ags & 0b1) ? 0b1 : 0b0;

			curr_ags >>= 1;
			real p1 = kmts[t];
#if defined(_SERIAL_CPU_MODE) || defined(_OPENMPI_MODE)
			small_idx_t i2_min = hw_findrange(p1, st_env.km, NKM_GRID);
			small_idx_t i2_max = i2_min + 1;
			real i2_min_val = st_env.km[i2_min];
			real i2_max_val = st_env.km[i2_max];
#else
			small_idx_t i2_min = hw_findrange((fixed_t)p1, fxd_km_grid, NKM_GRID);
			small_idx_t i2_max = i2_min + 1;
			real i2_min_val = km_grid[i2_min];
			real i2_max_val = km_grid[i2_max];
#endif
			real ty_num = (p1 - i2_min_val);
			real ty_den = (i2_max_val - i2_min_val);
			real ty = ty_num / ty_den;
			real P = (p0 == 1) ? 0 : (1.0 - ty);
			real Q = (p0 == 1) ? 0 : (ty);
			real R = (p0 == 1) ? (1.0 - ty) : 0;
			real S = (p0 == 1) ? (ty) : 0;
			small_idx_t i1_min_base = 0;	  // L4D_D3 * i1.min(0)
			small_idx_t i1_max_base = L4D_D3; // L4D_D3 * i1.max
			small_idx_t i2_min_base = L4D_D2 * i2_min;
			small_idx_t i2_max_base = L4D_D2 * i2_max;
			small_idx_t i12_min_min = i1_min_base + i2_min_base;
			small_idx_t i12_min_max = i1_min_base + i2_max_base;
			small_idx_t i12_max_min = i1_max_base + i2_min_base;
			small_idx_t i12_max_max = i1_max_base + i2_max_base;
			small_idx_t kpi_idx = 0;
		ast_2:
			for (int iid = 0; iid < NSTATES_ID; ++iid)
			{
				small_idx_t i3_min_base = 0;	  // L4D_D1 * i3.min (0)
				small_idx_t i3_max_base = L4D_D1; // L4D_D1 * i3.max (1)
				real tz = st_env.epsilon[iid];
			ast_3:
				for (int ik = 0; ik < NKGRID; ++ik)
				{
#pragma HLS pipeline
					int i4_min = ik;
					real p = (1.0 - tz);
					real r = tz;
					small_idx_t kp_idx_0 = i4_min + i3_min_base + i12_min_min;
					small_idx_t kp_idx_2 = i4_min + i3_max_base + i12_min_min;
					small_idx_t kp_idx_4 = i4_min + i3_min_base + i12_min_max;
					small_idx_t kp_idx_6 = i4_min + i3_max_base + i12_min_max;
					small_idx_t kp_idx_8 = i4_min + i3_min_base + i12_max_min;
					small_idx_t kp_idx_10 = i4_min + i3_max_base + i12_max_min;
					small_idx_t kp_idx_12 = i4_min + i3_min_base + i12_max_max;
					small_idx_t kp_idx_14 = i4_min + i3_max_base + i12_max_max;
// ** LI3D
#if ((NUM_KPRIMES == 4) || (NUM_KPRIMES == 8))
					real fp_1 = st_kprimes[0][kp_idx_0] * P * p;
					real fp_2 = st_kprimes[0][kp_idx_2] * P * r;
					real fp_3 = st_kprimes[1][kp_idx_4] * Q * p;
					real fp_4 = st_kprimes[1][kp_idx_6] * Q * r;
					real fp_5 = st_kprimes[2][kp_idx_8] * R * p;
					real fp_6 = st_kprimes[2][kp_idx_10] * R * r;
					real fp_7 = st_kprimes[3][kp_idx_12] * S * p;
					real fp_8 = st_kprimes[3][kp_idx_14] * S * r;
					real fp_9 = fp_1 + fp_2; 
					real fp_10 = fp_3 + fp_4; 
					real fp_11 = fp_5 + fp_6; 
					real fp_12 = fp_7 + fp_8; 
					real fp_13 = fp_9 + fp_10; 
					real fp_14 = fp_11 + fp_12; 
					real fp = fp_13 + fp_14; 
#elif (NUM_KPRIMES == 1)
					real fp = st_kprimes[0][kp_idx_0] * P * p +
							  st_kprimes[0][kp_idx_2] * P * r +
							  st_kprimes[0][kp_idx_4] * Q * p +
							  st_kprimes[0][kp_idx_6] * Q * r +
							  st_kprimes[0][kp_idx_8] * R * p +
							  st_kprimes[0][kp_idx_10] * R * r +
							  st_kprimes[0][kp_idx_12] * S * p +
							  st_kprimes[0][kp_idx_14] * S * r;
#endif
#if AST_UNROLL
					for (int k = 0; k < NUM_KCROSS; ++k)
					{
						kprime_interp0[k][kpi_idx] = fp;
						kprime_interp1[k][kpi_idx] = fp;
					}
#else
					kprime_interp0[kpi_idx] = fp;
					kprime_interp1[kpi_idx] = fp;
#endif
					++kpi_idx;
				} //end ast_3
			} //end ast_2

			//aggregate capital initialized to 0
			kc_t agg_capital = 0;

#if AST_UNROLL
			small_idx_t kidx = 0;
// Loop 1.3: AST agents interp over kprime_interp
// Unroll factor dictated by inner loop over k
#if PACK_IDS
			idshock_cnt = 8;
#endif
		ast_4:
			for (int j = 0; j < (N_AGENTS / IDS_PACK_FACTOR) / IDS_AGG_X; j++)
			{
#pragma HLS pipeline
#if PACK_IDS
				if (idshock_cnt >= 8)
				{
					idshock_cnt = 0;
					temp_ids = idshock[idshock_idx];
					idshock_idx++;
				}
				curr_ids = temp_ids & 0xFF;
				idshock_cnt++;
				temp_ids >>= 8;
#else
				curr_ids = idshock[idshock_idx++];
#endif

			ast_5:
				for (int k = 0; k < IDS_PACK_FACTOR * IDS_AGG_X; ++k)
				{
					real p1b = st_kcross[kidx];
					small_idx_t i2b_min = hw_findrange((fixed_t)st_kcross[kidx], fxd_k_grid, NKGRID);
					small_idx_t i2b_max = i2b_min + 1;
					real i2b_min_val = k_grid[i2b_min];
					real i2b_max_val = k_grid[i2b_max];
					bool p0b = (curr_ids & 0b1) ? 0b1 : 0b0;
					curr_ids >>= 1;
					small_idx_t i1b_min_base = 0;	   // NKGRID * i1b_min(0)
					small_idx_t i1b_max_base = NKGRID; // NKGRID * i1b_max(1)
					real bw_num = (p1b - i2b_min_val);
					real bw_den = (i2b_max_val - i2b_min_val);
					real bw = bw_num / bw_den;
					real sub_bw = (1.0 - bw);
					real bz_bw = (p0b == 1) ? bw : 0;
					real sub_bz_sub_bw = (p0b == 1) ? 0 : sub_bw;
					real bz_sub_bw = (p0b == 1) ? sub_bw : 0;
					real bw_sub_bz = (p0b == 1) ? 0 : bw;
					real fpb_1 = (kprime_interp0[k][i1b_min_base + i2b_min] * sub_bz_sub_bw);
					real fpb_2 = (kprime_interp0[k][i1b_min_base + i2b_max] * bw_sub_bz);
					real fpb_3 = (kprime_interp1[k][i1b_max_base + i2b_min] * bz_sub_bw);
					real fpb_4 = (kprime_interp1[k][i1b_max_base + i2b_max] * bz_bw);
					real fpb_5 = fpb_1 + fpb_2;
					real fpb_6 = fpb_3 + fpb_4;
					kc_t fpb = kc_t(fpb_5 + fpb_6);
					hw_fxd_rail_values(&fpb, KMAX, KMIN);
					st_kcross[kidx] = (real)fpb;
					agg_capital += fpb;
					kidx++;
				} //end ast_5
			} //end ast_4
#else
			small_idx_t ids_phase = IDS_PACK_FACTOR;
		// Loop 1.3: AST agents interp over kprime_interp
		ast_4:
			for (int j = 0; j < N_AGENTS; ++j)
			{
#pragma HLS pipeline
// Read next packed idshock value when needed
#if PACK_IDS
				if (idshock_cnt >= (8 * IDS_PACK_FACTOR))
				{
					if (j == 9984)
					{
						// count only remaining 16 and then move to next row
						idshock_cnt = 48;
					}
					else
					{
						idshock_cnt = 0;
					}
					temp_ids = idshock[idshock_idx];
					idshock_idx++;
				}
				curr_ids = (unsigned char)(temp_ids & (ap_uint<72>)0x01);
				idshock_cnt++;
				real p0b = (curr_ids & 0b1) ? (real)1.0 : (real)0.0;
				temp_ids >>= 1;
#else
				if (++ids_phase >= IDS_PACK_FACTOR)
				{
					curr_ids = idshock[idshock_idx++];
					ids_phase = 0;
				}
				real p0b = (curr_ids & 0b1) ? (real)1.0 : (real)0.0;
				curr_ids >>= 1;
#endif
				real p1b = st_kcross[j];
#if defined(_SERIAL_CPU_MODE) || defined(_OPENMPI_MODE)
				small_idx_t i2b_min = hw_findrange(p1b, st_env.k, NKGRID);
				small_idx_t i2b_max = i2b_min + 1;
				real i2b_min_val = st_env.k[i2b_min];
				real i2b_max_val = st_env.k[i2b_max];
#else
				small_idx_t i2b_min = hw_findrange((fixed_t)p1b, fxd_k_grid, NKGRID);
				small_idx_t i2b_max = i2b_min + 1;
				real i2b_min_val = k_grid[i2b_min];
				real i2b_max_val = k_grid[i2b_max];
#endif
				small_idx_t i1b_min_base = 0;	   // NKGRID * i1b.min always 0
				small_idx_t i1b_max_base = NKGRID; // NKGRID * i1b.max always NKGRID
				real bw_num = (p1b - i2b_min_val);
				real bw_den = (i2b_max_val - i2b_min_val);
				real bw = bw_num / bw_den;
				real sub_bw = (1.0 - bw);
				real bz_bw = (p0b == 1) ? bw : 0;
				real sub_bz_sub_bw = (p0b == 1) ? 0 : sub_bw;
				real bz_sub_bw = (p0b == 1) ? sub_bw : 0;
				real bw_sub_bz = (p0b == 1) ? 0 : bw;
				real fpb_1 = (kprime_interp0[i1b_min_base + i2b_min] * sub_bz_sub_bw);
				real fpb_2 = (kprime_interp0[i1b_min_base + i2b_max] * bw_sub_bz);
				real fpb_3 = (kprime_interp1[i1b_max_base + i2b_min] * bz_sub_bw);
				real fpb_4 = (kprime_interp1[i1b_max_base + i2b_max] * bz_bw);
				real fpb_5 = fpb_1 + fpb_2;
				real fpb_6 = fpb_3 + fpb_4;
				kc_t fpb = kc_t(fpb_5 + fpb_6);
				hw_fxd_rail_values(&fpb, KMAX, KMIN);
				st_kcross[j] = (real)fpb;
				agg_capital += fpb;
			} //end ast_4
#endif
			curr_kmts = ((real)agg_capital * N_AGENTS_INV);
			hw_rail_values(&curr_kmts, KM_MAX, KM_MIN);
		} // end simulation steps (ast_1)
#if PRINT_LOOP_CNT
		printf("ast loop cnt = %d\n", iter_cnt);
#endif
		return;
	}

	void sim_alm_coeff(const real *kmts, real *coeff_updated, real *metric,
					   real *R2, unsigned char agshock[AGSHOCK_ARR_SIZE])
	{
		real coeff[NCOEFF] = {0.};
	sim_alm_1:
		for (small_idx_t i = 0; i < NCOEFF; i++)
		{
#pragma HLS pipeline off
			coeff[i] = coeff_updated[i];
		}
		small_idx_t agshock_idx = 0;
		small_idx_t ags_phase = AGS_PACK_FACTOR;
		shock_t curr_ags = 0;
		shock_t curr_shock_val = 0;
		real coeff_new[NCOEFF] = {0.};
		real x_good_v[1000] = {0.};
		real y_good_v[1000] = {0.};
		real x_bad_v[1000] = {0.};
		real y_bad_v[1000] = {0.};

		int ibad = 0;
		int igood = 0;
		agshock_idx = 0;
		ags_phase = AGS_PACK_FACTOR;
	sim_alm_2:
		for (int t = 0; t < SIM_STEPS; t++)
		{
#pragma HLS pipeline off
#pragma HLS unroll factor = 1
			// Read new value when needed
			if (++ags_phase >= AGS_PACK_FACTOR)
			{
				curr_ags = agshock[agshock_idx++];
				ags_phase = 0;
			}
			curr_shock_val = curr_ags & 0b1; // take the least significant bit from the byte
			curr_ags >>= 1;					 // right shift by 1
		// Discard first 100
		sim_alm_3:
			if (t < NDISCARD || t > SIM_STEPS - 2)
				continue;
		sim_alm_4:
			if (curr_shock_val == 0)
			{
				y_bad_v[ibad] = hw_log(kmts[t + 1]);
				x_bad_v[ibad] = hw_log(kmts[t]);
				ibad++;
			}
			else
			{
				y_good_v[igood] = hw_log(kmts[t + 1]);
				x_good_v[igood] = hw_log(kmts[t]);
				igood++;
			}
		}
		real badcoeff[2] = {0.}; // initialize to prevent garbage values
		real goodcoeff[2] = {0.};
		regression(badcoeff, x_bad_v, y_bad_v, ibad);
		regression(goodcoeff, x_good_v, y_good_v, igood);
		real rbad = RSquaredCalc(badcoeff, x_bad_v, y_bad_v, ibad);
		real rgood = RSquaredCalc(goodcoeff, x_good_v, y_good_v, igood);
		coeff_new[0] = badcoeff[0]; // bb
		coeff_new[1] = badcoeff[1];
		coeff_new[2] = goodcoeff[0];
		coeff_new[3] = goodcoeff[1];
		R2[0] = rbad;
		R2[1] = rgood;
		// Update metric for convergence test (eq 17)
		real norm = 0.;
	sim_alm_5:
		for (int ib = 0; ib < NCOEFF; ++ib)
		{
#pragma HLS pipeline off
			norm += (coeff_new[ib] - coeff[ib]) * (coeff_new[ib] - coeff[ib]);
		}
		*metric = hw_sqrt(norm);
	// Update ALM coefficients vector
	sim_alm_6:
		for (int ib = 0; ib < NCOEFF; ++ib)
		{
#pragma HLS pipeline off
			coeff_updated[ib] = coeff_new[ib] * UPDATE_B + coeff[ib] * (1. - UPDATE_B); // 
		}
		return;
	}

	void regression(real *resultmatrix, real *x, real *y, int ndim)
	{
		real twobytwo[4] = {0, 0, 0, 0};
	RG_1:
		for (int i = 0; i < ndim; i++)
		{
#pragma HLS loop_tripcount min = 100 avg = 494 max = 1000
#pragma HLS unroll factor = 1
#pragma HLS pipeline off
			twobytwo[0] += 1;
			twobytwo[1] += x[i];
			twobytwo[2] += x[i];
			// twobytwo[3] += hw_pow(x[i], 2);
			real temp_x_square = x[i]*x[i];
			twobytwo[3] += temp_x_square;
		}
		// get inverse
		real a = twobytwo[0]; // switching indices and multiplying by determinant
		real b = twobytwo[1];
		real c = twobytwo[2];
		real d = twobytwo[3];
		real a_d = a*d;
		real b_c = b*c;
		real det = (a_d - b_c);

		real inv_det = (1.0 / det);
		real inv_d = inv_det * d;
		real inv_b = inv_det * (b) * -1; //optimize
		real inv_c = inv_det * (c) * -1; //optimize
		real inv_a = inv_det * a;
		real acc1 = resultmatrix[0];
		real acc2 = resultmatrix[1];
	// multiply by transpose of matrix and y
	RG_2:
		for (int i = 0; i < ndim; i++)
		{
#pragma HLS loop_tripcount min = 100 avg = 494 max = 1000
#pragma HLS unroll factor = 1
#pragma HLS pipeline off
			real acc_t1 = inv_b * x[i];
			real acc_t2 = inv_d + acc_t1;
			acc1 += acc_t2 * y[i];
		}
		resultmatrix[0] = acc1;
	RG_3:
		for (int i = 0; i < ndim; i++)
		{
#pragma HLS loop_tripcount min = 100 avg = 494 max = 1000
#pragma HLS unroll factor = 1
#pragma HLS pipeline off
			real acc2_t1 = inv_a * x[i];
			real acc2_t2 = inv_c + acc2_t1;
			acc2 += acc2_t2 * y[i];
		}
		resultmatrix[1] = acc2;
		return;
	}

	real RSquaredCalc(real *coeff, real *x, real *y, int ndim)
	{
		real r_value = 0;
		real predict[1000] = {0};
		real rss = 0;
		real tss = 0;
		real y_mean = 0;
	R2_1:
		for (int i = 0; i < ndim; i++)
		{
#pragma HLS pipeline off
#pragma HLS unroll factor = 1
#pragma HLS loop_tripcount min = 100 avg = 494 max = 1000
			y_mean += y[i];
		}
		y_mean = (y_mean / ndim);

	R2_2:
		for (int i = 0; i < ndim; i++)
		{
#pragma HLS pipeline off
#pragma HLS unroll factor = 1
#pragma HLS loop_tripcount min = 100 avg = 494 max = 1000
			real predict_temp = (coeff[1] * x[i]);
			predict[i] = (coeff[0] + predict_temp);
			real temp_rss = (predict[i] - y[i]);
			rss += temp_rss*temp_rss;
			// rss += hw_pow((predict[i] - y[i]), 2);
			real temp_tss = (y[i] - y_mean);
			tss += (temp_tss*temp_tss);
			// tss += hw_pow((y[i] - y_mean), 2);
		}
		r_value = (1.0 - (rss / tss));

		return r_value;
	}

	void hw_init_env()
	{
#pragma HLS inline
		st_env.alpha = env__alpha;	   // 0.36 (Output capital share)
		st_env.beta = env__beta;	   // 0.99 (Quarterly subjective discount factor)
		st_env.delta = env__delta;	   // 0.025 (Quarterly depreciation rate)
		st_env.mu = env__mu;		   // 0.15 (Unemployment benefits in terms of wages)
		st_env.l_bar = env__l_bar;	   // 
		st_env.delta_a = env__delta_a; // 0.01

		st_env.l_bar_inv = env__l_bar_inv; // 0.9 (Time endowment) ?
		st_env.gamma_inv = env__gamma_inv;
		st_env.gamma_neg = env__gamma_neg;
		st_env.gamma_neg_inv = env__gamma_neg_inv;

		st_env.epsilon_u = env__epsilon_u;
		st_env.epsilon_e = env__epsilon_e;

		st_env.ur[0] = env__ur_0;
		st_env.er[0] = (1 - st_env.ur[0]);
		st_env.ur[1] = env__ur_1;
		st_env.er[1] = (1 - st_env.ur[1]);

		st_env.er_inv[0] = 1 / st_env.er[0];
		st_env.er_inv[1] = 1 / st_env.er[1];

		// st_env.kss = hw_pow((1./st_env.beta-(1.-st_env.delta))/st_env.alpha,1./(st_env.alpha-1));
		st_env.kss = env__kss;

		// transition
		st_env.P[0] = 0.525;
		st_env.P[1] = 0.35;
		st_env.P[2] = 0.03125;
		st_env.P[3] = 0.09375;
		st_env.P[4] = 0.038889;
		st_env.P[5] = 0.836111;
		st_env.P[6] = 0.002083;
		st_env.P[7] = 0.122917;
		st_env.P[8] = 0.09375;
		st_env.P[9] = 0.03125;
		st_env.P[10] = 0.291667;
		st_env.P[11] = 0.583333;
		st_env.P[12] = 0.009115;
		st_env.P[13] = 0.115885;
		st_env.P[14] = 0.024306;
		st_env.P[15] = 0.850694;

		// parmshocks
		st_env.epsilon[0] = st_env.epsilon_u;
		st_env.epsilon[1] = st_env.epsilon_e;
#if AST_UNROLL
		for (int k = 0; k < NUM_KCROSS; ++k)
		{
#pragma HLS pipeline off
			st_env.epsilon2[k][0] = 0;
			st_env.epsilon2[k][1] = 1;
		}
#else
		st_env.epsilon2[0] = 0;
		st_env.epsilon2[1] = 1;
#endif

		st_env.ag[0] = 1 - st_env.delta_a;
		st_env.ag[1] = 1 + st_env.delta_a;
		st_env.ag2[0] = 0;
		st_env.ag2[1] = 1;
		return;
	}

	real hw_exp(real b)
	{
#pragma HLS inline
#if USE_HLS_LIB
		return hls::exp(b);
#else
		return exp(b);
#endif
	}

	real hw_log(real b)
	{
#pragma HLS inline
#if USE_HLS_LIB
		return hls::log(b);
#else
		return log(b);
#endif
	}

	real hw_sqrt(real b)
	{
#pragma HLS inline
#if USE_HLS_LIB
		return hls::sqrt(b);
#else
		return sqrt(b);
#endif
	}

	real hw_fabs(real b)
	{
#pragma HLS inline
#if USE_HLS_LIB
		return hls::fabs(b);
#else
		return fabs(b);
#endif
	}

	real hw_pow(real x, real y)
	{
#pragma HLS inline
#if USE_HLS_LIB
		return hls::pow(x, y);
#else
		return pow(x, y);
#endif
	}

	void hw_rail_values(real *val, const real max, const real min)
	{
#pragma HLS inline
		real src = *val;
		bool over_max = (src > max);
		bool under_min = (src < min);

	hw_rail_1:
		if (over_max)
			*val = max;
		else if (under_min)
			*val = min;
		return;
	}

	void hw_fxd_rail_values(kc_t *val, const real max, const real min)
	{
#pragma HLS inline
		kc_t src = *val;
		bool over_max = ((real)src > max);
		bool under_min = ((real)src < min);

		if (over_max)
			*val = max;
		else if (under_min)
			*val = min;
		return;
	}

	small_idx_t hw_findrange_n4(fixed_t p, const fixed_t *src)
	{
#pragma HLS pipeline
		small_idx_t result;

	fr4_1:
		for (signed short i = 3; i > 0; --i)
		{
		fr4_2:
			if (p <= src[i])
			{
				result = i - 1;
			}
		}
		return result;
	}

	small_idx_t hw_findrange_n8(fixed_t p, const fixed_t *src)
	{
#pragma HLS pipeline
		small_idx_t result;

		for (signed short i = 7; i > 0; --i)
		{
			if (p <= src[i])
			{
				result = i - 1;
			}
		}
		return result;
	}

	small_idx_t hw_findrange_n100(fixed_t p, const fixed_t *src)
	{
#pragma HLS pipeline
		small_idx_t result_1 = 0;
		small_idx_t result_2 = 0;
		small_idx_t result_3 = 0;
		small_idx_t result = 0;

	fr100_1:
		for (signed short i = 99; i > 0; i=i-10) //10 comparators
		{
		fr100_2:
			if (p <= src[i])
			{
				result_1 = i; //send the max idex
			}
		}

	fr100_3:
		for (signed short i = 2; i > 0; i--) // 2 comparators
		{
		fr100_4:
			if (p <= src[result_1])
			{
				result_2 = result_1; //send the max index
			}
			result_1 = result_1 - (small_idx_t)5;
		}

	fr100_5:
		for (signed short i = 5; i > 0; i--) //5 comparators
		{
		fr100_6:
			if (p <= src[result_2--])
			{
				result_3 = result_2; //send the min index
			}
		}

		result = (p==src[0]) ? (small_idx_t)0 : result_3; 
		return result;
	}

	small_idx_t hw_findrange_n200(fixed_t p, const fixed_t *src)
	{
#pragma HLS pipeline
		small_idx_t result_1 = 0;
		small_idx_t result_2 = 0;
		small_idx_t result_3 = 0;
		small_idx_t result = 0;

	fr200_1:
		for (signed short i = 199; i > 0; i=i-10) //20 comparators
		{
		fr200_2:
			if (p <= src[i])
			{
				result_1 = i; //send the max idex
			}
		}

	fr200_3:
		for (signed short i = 2; i > 0; i--) // 2 comparators
		{
		fr200_4:
			if (p <= src[result_1])
			{
				result_2 = result_1; //send the max index
			}
			result_1 = result_1 - (small_idx_t)5;
		}

	fr200_5:
		for (signed short i = 5; i > 0; i--) //5 comparators
		{
		fr200_6:
			if (p <= src[result_2--])
			{
				result_3 = result_2; //send the min index
			}
		}

		result = (p==src[0]) ? (small_idx_t)0 : result_3; 
		return result;
	}

	small_idx_t hw_findrange_n300(fixed_t p, const fixed_t *src)
	{
#pragma HLS pipeline
		small_idx_t result_1 = 0;
		small_idx_t result_2 = 0;
		small_idx_t result_3 = 0;
		small_idx_t result = 0;

	fr300_1:
		for (signed short i = 299; i > 0; i=i-10) //30 comparators
		{
		fr300_2:
			if (p <= src[i])
			{
				result_1 = i; //send the max idex
			}
		}

	fr300_3:
		for (signed short i = 2; i > 0; i--) // 2 comparators
		{
		fr300_4:
			if (p <= src[result_1])
			{
				result_2 = result_1; //send the max index
			}
			result_1 = result_1 - (small_idx_t)5;
		}

	fr300_5:
		for (signed short i = 5; i > 0; i--) //5 comparators
		{
		fr300_6:
			if (p <= src[result_2--])
			{
				result_3 = result_2; //send the min index
			}
		}

		result = (p==src[0]) ? (small_idx_t)0 : result_3; 
		return result;
	}

	small_idx_t hw_findrange(fixed_t p, const fixed_t *src, int n_elem)
	{
#if _LINEAR_SEARCH || _BASELINE
		small_idx_t result = 1;
		for (signed short i = (n_elem - 1); i > 0; --i)
		{
			if (p <= src[i])
			{
				result = i - 1;
			}
		}
		return result;
#elif _BINARY_SEARCH
#ifdef _FPGA_MODE
		printf("ERROR: No support for binary search in FPGA"\n);
#else
        small_idx_t l = 0;
        small_idx_t r = n_elem - 1; //last index for array with 100 elements
        while(l <= r){

            small_idx_t m = (l + r) >> 1; //div by 2

            // If x greater, ignore left half  
            if (src[m]<= p) 
                l = m + 1; 

            // If x is smaller, ignore right half 
            else{
                r = m - 1; 
            } 
            
        }

		r = (p==src[0]) ? 0 : r; 

		return r;
#endif // _FPGA_MODE		
#elif _CUSTOM_BINARY_SEARCH || (_PIPELINE || _WITHIN_ECONOMY || _ACROSS_ECONOMY)
#pragma HLS inline
#if (NKM_GRID == 4)
		if (n_elem == 4)
			return hw_findrange_n4(p, src);
#elif (NKM_GRID == 8)
		if (n_elem == 8)
			return hw_findrange_n8(p, src);
#endif
#if (NKGRID == 100)
		else if (n_elem == 100)
			return hw_findrange_n100(p, src);
#elif (NKGRID == 200)
		else if (n_elem == 200)
			return hw_findrange_n200(p, src);
#elif (NKGRID == 300)
		else if (n_elem == 300) 
		return hw_findrange_n300(p, src);
#endif
		else
			return 0;

#endif
	}
}