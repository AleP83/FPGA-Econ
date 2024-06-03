#pragma once
#include <stdio.h>
#include <stdlib.h> // must be prior to sds_lib.h
#include <string.h>
#include <assert.h>
// #include <cstdint>

/** Development options */
#include "dev_options.h"

/** Hardware includes */
#ifdef _FPGA_MODE
#include <ap_int.h>
#include <ap_fixed.h>
#endif
/** Typedefs */
typedef double real;
typedef unsigned short small_idx_t;
typedef unsigned int idx_t;
typedef unsigned char shock_t; 

#if FIXED_ACC
typedef ap_fixed<72, 21> fixed_t;    // ap_[u]fixed<W,I,Q,O,N> where W is word length in bits and I is no. of bits above the decimal point
typedef ap_fixed<72, 21> emu_s_t;    // fixed precision
typedef ap_fixed<72, 21> kc_t;       // data type for agg capital calc
#if SPREAD_SCALAR_TYPE
typedef ap_fixed<72, 16> spread_t;   // data type for agg capital calc
#else
typedef double spread_t;             // data type for agg capital calc
#endif
#else
typedef double fixed_t;
typedef double emu_s_t;               // data type for expectation term in IAP
typedef double kc_t;                  // data type for agg capital calc
typedef double spread_t;              // data type for agg capital calc
#endif

/** Configure grid points. 
The analysis in the paper involves six distinct combinations: NKGRID={100,200,300}; NKM_GRID={4,8}.
*/
#define NKGRID 100	 // grid points on individual capital grid
#define NKM_GRID 4	 // grid points on aggregate capital grid

/** Paths */
#define SHOCKPATH "shocks/"
#define KPRIME_FILE_NAME "kprime.txt"
#define IDSHOCK_FILE_NAME "idshock.txt"
#define AGSHOCK_FILE_NAME "agshock.txt"
#define IDSHOCK_BIN_FILE "idshock.dat"
#define AGSHOCK_BIN_FILE "agshock.dat"

/** Programming constants */
#define MAX_FILENAME_LEN 150
#define MAX_EGM_ITER 100

/** Auxiliary constants */
#define XMIN ((real)0.0)
#define XMAX ((real)0.5)
#define BYTE_PACK_FACTOR 8 // 8 bits to byte
#define AGS_PACK_FACTOR 8  // data width may change
#define IDS_PACK_FACTOR 8  // data width may change

/** Simulation settings */
#define N_AGENTS 10000												// number of agents in stochastic simulation
#define N_AGENTS_INV ((real)0.0001)									// (1. / N_AGENTS)
#define SIM_STEPS 1100												// simulation length
#define AGSHOCK_ARR_SIZE 138										// (ceil(1100/8))
#define IDSHOCK_ARR_SIZE ((N_AGENTS / IDS_PACK_FACTOR) * SIM_STEPS) // 1375000
// Better name above as "number of bytes"
#define POWER_POLYNOMIAL_KGRID (7.) // Exponent of polynomial rule on capital grid

#if IDS_AGG_PACK
#define IDSHOCK_HW_ARR_SIZE ((N_AGENTS / IDS_PACK_FACTOR) * SIM_STEPS) / IDS_AGG_X
#else
#define IDSHOCK_HW_ARR_SIZE ((N_AGENTS / IDS_PACK_FACTOR) * SIM_STEPS)
#endif
#define NDISCARD 100 // Periods to discard

/** Application constants */
#define NSTATES_ID 2 // number of states for the idiosyncratic shock
#define NSTATES_AG 2 // number of states for the aggregate shock
#define NCOEFF 4	 // number of ceofficient estimates
#define NSTATES (NSTATES_ID * NSTATES_AG * NKGRID * NKM_GRID)
#define L4D_D3 (NKM_GRID * NSTATES_ID * NKGRID)
#define L4D_D2 (NSTATES_ID * NKGRID)
#define L4D_D1 NKGRID

#define SIZE_P 16
#define KMIN (0.0)              // minimum grid-value of capital
#define KMAX (1000.0)           // maximum grid-value of capital
#define NKPRIME_SCALE (0.9)     // scaling factor inititalization kprime cross-sectional distribution
#define CONS2_MIN (0.0000000001) // pow(10., -10.);
#define KM_MIN (30.0)           // minimum grid-value of the mean of capital distribution, km
#define KM_MAX (50.0)           // maximum grid-value of the mean of capital distribution, km


#define MF_LOOP_ITER 1000 // maximum loop count of matrixfunction
#define VERY_SMALL_SCALAR (0.0000000001)

/** Environment variables */
#define env__alpha (0.36)               // 0.36 (Output capital share)
#define env__alpha_c ((-0.64))          // alpha - 1
#define env__beta (0.99)                // 0.99 (Quarterly subjective discount factor)
#define env__gamma (1.0)                // 1 (Arrow-Pratt relative risk aversion coefficient)
#define env__gamma_neg ((-1.0))         //
#define env__gamma_neg_inv ((-1.0))     //
#define env__gamma_inv (1.0)            // 1
#define env__delta (0.025)              // 0.025 (Quarterly depreciation rate)
#define env__delta_c (0.975)            // 1. - delta
#define env__mu (0.15)                  // 0.15 (Unemployment benefits in terms of wages)
#define env__l_bar (1.1111111111111112) // 1/0.9
#define env__l_bar_inv (0.9)            // 0.9 (Time endowment)
#define env__delta_a (0.01)             // 0.01 (Aggregate productivity shock size)
#define env__epsilon_u (0.0)
#define env__epsilon_e (1.0)
#define env__epsilon_0 (0.0)
#define env__epsilon_1 (1.0)
#define env__ur_0 (0.1)
#define env__er_0 (0.9)
#define env__ur_1 (0.04)
#define env__er_1 (0.96)
#define env__er_inv_0 (1.1111111111111112)
#define env__er_inv_1 ((1 / 0.96))
#define env__kss (37.9892535815222)

/** Macros */
#define IXP(a, i, ap, ip) (NSTATES_ID * NSTATES_AG * NSTATES_ID * a + NSTATES_AG * NSTATES_ID * i + NSTATES_ID * ap + ip)
#define IXEV(a, i, j) (NSTATES_ID * NKGRID * a + NKGRID * i + j)
#define IXV(a, km, id, k) (NKM_GRID * NSTATES_ID * NKGRID * a + NSTATES_ID * NKGRID * km + NKGRID * id + k)
#define IX(t, j) (t * N_AGENTS + j)
#define IXID(i, k) (i * NKGRID + k)
#define IXAKM(a, km) (a * NKM_GRID + km)
#define IXAI(a, i) (a * NSTATES_ID + i)

/** Convergence */
#define TOLL_K ((real)(1e-8))
#define TOLL_COEFF ((real)(1e-8))

#define UPDATE_K (0.7)   // updating parameter for the individual capital function
#define UPDATE_K_C (0.3) //
#define UPDATE_B (0.3)   // updating parameter for the coefficients B in the ALM

#define REGRESSORS 2

/** Containers for parameters */
typedef struct env_t
{
	// full: 1618+16+10+100+4+100+4+14
	real alpha;									// share of the capital in production {0-1}
	real beta;									// discount factor {0-1}
	real delta;									// depreciation rate of capital {0-1}
	real gamma;									// utility-function parameter {>0}
	real mu;									// unemployment benefits as a share of wage
	real l_bar;									// time endowment; normalizes labor supply to 1 in a bad state
	real l_bar_inv;								// intermediate results
	real gamma_inv;								// intermediate results
	real gamma_neg;								// intermediate results
	real gamma_neg_inv;							// intermediate results
	real delta_a;								// (1-delta_a) is the productivity level in a bad state, and (1+delta_a) is the productivity level in a good state
	real epsilon_u;								// idiosyncratic shock if the agent is unemployed
	real epsilon_e;								// idiosyncratic shock if the agent is employed
	real kss;									// steady-state capital in a deterministic model with employment rate of 0.9 (i.e., l_bar*L=1 where L is aggregate labor in the paper)
	real km[NKM_GRID];							// grid of mean of capital distribution
	real k[NKGRID];								// grid of capital
	real epsilon2[NSTATES_ID];					// employed idiosyncratic shock?
	real trate[NSTATES_AG];						// labor-income tax rate
	real ag[NSTATES_AG];						// aggregate productivity shock
	real ag2[NSTATES_AG];						// What is this for?
	real epsilon[NSTATES_ID];					// unemployed idiosyncratic shock?
	real P[SIZE_P];								// matrix of transiiton probabilities
	real wealth[NSTATES];						// intermediate result for capital tomm
	real ur[NSTATES_AG];						// unemployment rate for aggregate states
	real er[NSTATES_AG];						// employment rate for bad aggregate states
	real er_inv[NSTATES_AG];					//
	real irate_factor[NSTATES_AG];				//
	real wage_factor[NSTATES_AG];				//
	real cons2_factor[NSTATES_AG * NSTATES_ID]; //
	real log_env_km[NKM_GRID];					//
} env_t;

/**
 * @brief Container for inputs
 *
 * Stores randomly generated indiviual shocks
 * and aggregate shocks.
 */
typedef struct input_t
{
	shock_t *idshock;		// array of size 1,375,000
	unsigned char *agshock; // array of size 138 ceil(1100/8)
} input_t;

/**
 * @brief Container for variables states
 *
 *
 */
typedef struct vars_t
{
	real kmprime[NKM_GRID]; // aggregate capital for (2x4)states
	real kprime_a[NSTATES]; // capital function for t=0
	// real kprime_b[NSTATES];	 // new capital function for t=1
	real kcross_l[N_AGENTS]; // individual capital for 1st iteration
	real kcross[N_AGENTS];	 // individual capital for 1st iteration
	real coeff[NCOEFF];		 // to hold good & bad, alpha, beta
	real coeff1[NCOEFF];	 // used in old file?
	real kmts[SIM_STEPS];	 // moments to charecterize capital distribution
} vars_t;

/**
 * @brief final results to be read back
 *
 */
typedef struct out_t
{
	real kprime[NSTATES];
	real kcross[N_AGENTS];
	real coeff[NCOEFF];
	real r2[NSTATES_AG];
} out_t;

/**
 * @brief pre initialized values to device
 *
 */
typedef struct preinit_t
{
	real kprime[NSTATES];
	real wealth[NSTATES];
#ifndef _FPGA_MODE
	real k[NKGRID];	
	real km[NKM_GRID];
#endif
} preinit_t;