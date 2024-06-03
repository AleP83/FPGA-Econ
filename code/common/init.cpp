#include "init.h"
#include "definitions.h"

#if SHOCKS_FROM_HEADER
#include "cons.h"
#endif

// put constants in the bitstream? maybe declare as static
// initialization memory arrays (Hans has some experience)
// fpga rand generation distr check how they're doing this currently
// Imperial black shoals
// Rewrite packed file
// Probably use OCM
// Probably run it very fast on the ARM core (see if we can just use seed)
// Many of these are just LFSR

void linear_space(const real xmin,
				  const real xmax,
				  const int ngrid,
				  real *grid)
{
	real increment;
	increment = (real) ((xmax) - (xmin)) / (ngrid - 1);
	for (int i = 0; i < ngrid; ++i)
		grid[i] = (xmin) + i * increment;
	return;
}

void init_environment(env_t *env)
{
	env->beta = 0.99;
	env->gamma = 1;
	env->alpha = 0.36;
	env->delta = 0.025;
	env->delta_a = 0.01;
	env->mu = 0.15;
	env->l_bar = 1 / 0.9;

	env->l_bar_inv = 0.9;
	env->gamma_inv = 1;
	env->gamma_neg = -1;
	env->gamma_neg_inv = -1;

	env->epsilon_u = 0;
	env->epsilon_e = 1;

	env->ur[0] = 0.1;
	env->er[0] = (1 - env->ur[0]);
	env->ur[1] = 0.04;
	env->er[1] = (1 - env->ur[1]);

	env->er_inv[0] = 1/env->er[0];
	env->er_inv[1] = 1/env->er[1];

	env->kss = pow((1./env->beta-(1.-env->delta))/env->alpha,1./(env->alpha-1)); 

	return;
}

void init_transition(env_t *env)
{
	env->P[IXP(0, 0, 0, 0)] = 0.525;
	env->P[IXP(0, 0, 0, 1)] = 0.35;
	env->P[IXP(0, 0, 1, 0)] = 0.03125;
	env->P[IXP(0, 0, 1, 1)] = 0.09375;
	env->P[IXP(0, 1, 0, 0)] = 0.038889;
	env->P[IXP(0, 1, 0, 1)] = 0.836111;
	env->P[IXP(0, 1, 1, 0)] = 0.002083;
	env->P[IXP(0, 1, 1, 1)] = 0.122917;
	env->P[IXP(1, 0, 0, 0)] = 0.09375;
	env->P[IXP(1, 0, 0, 1)] = 0.03125;
	env->P[IXP(1, 0, 1, 0)] = 0.291667;
	env->P[IXP(1, 0, 1, 1)] = 0.583333;
	env->P[IXP(1, 1, 0, 0)] = 0.009115;
	env->P[IXP(1, 1, 0, 1)] = 0.115885;
	env->P[IXP(1, 1, 1, 0)] = 0.024306;
	env->P[IXP(1, 1, 1, 1)] = 0.850694;
}

/**
 * Reads binary shock inputs and stores them to struct
 */
void init_binary_inputs(input_t *in)
{
#if SHOCKS_FROM_HEADER
	memcpy(in->agshock, ags_dat, AGSHOCK_ARR_SIZE * sizeof(unsigned char));
	memcpy(in->idshock, ids_dat, IDSHOCK_ARR_SIZE * sizeof(unsigned char));
#else
	/**
	 * Difference between uint32_t and uint8_t version of idshock :
	 * After copy, uint8_t [0:3]:
	 *	0xf7 0xff 0xad 0xff
	 * After copy, uint32_t [0:3]:
	 *	0xffadfff7
	 * When processing as uint8_t:
	 *	[7..0] corresponds to [7..0]
	 * Thus, when processing as uint32_t:
	 *	[31..24]
	 *	[23..16]
	 *	[15..8]
	 *	[7..0]
	 */
	// This portion of the code assumes unsigned char for shock arrays!
	// Use SHOCKS_FROM_HEADER when IDS_AGG_PACK
	char fn[MAX_FILENAME_LEN];
	unsigned int res = 0;

	// Read agshock
	strcpy(fn, SHOCKPATH);
	strcat(fn, AGSHOCK_BIN_FILE);
	if ((res = read_bin_dat(fn, in->agshock, AGSHOCK_ARR_SIZE)) < 0) {
		fprintf(stderr, "ERR: %d while reading %s\n", res, fn);
		exit(EXIT_FAILURE);
	} else {
		fprintf(stderr, "Open: %s [%d bytes read]\n", fn, res);
	}

	// Read idshock
	strcpy(fn, SHOCKPATH);
	strcat(fn, IDSHOCK_BIN_FILE);
	if ((res = read_bin_dat(fn, in->idshock, IDSHOCK_ARR_SIZE)) < 0) {
		fprintf(stderr, "ERR: %d while reading %s\n", res, fn);
		exit(EXIT_FAILURE);
	} else {
		fprintf(stderr, "Open: %s [%d bytes read]\n", fn, res);
	}
#endif


	return;
}

void init_parmshocks(env_t *env)
{
	env->epsilon[0] = env->epsilon_u;
	env->epsilon[1] = env->epsilon_e;

	env->epsilon2[0] = 0;
	env->epsilon2[1] = 1;

	env->ag[0] = 1 - env->delta_a;
	env->ag[1] = 1 + env->delta_a;

	env->ag2[0] = 0;
	env->ag2[1] = 1;
}

void init_prealloc(env_t *env, vars_t *vars)
{
	for (int ia = 0; ia < NSTATES_AG; ++ia) {
		env->trate[ia] = env->mu * (1.-env->er[ia]) / (env->l_bar*env->er[ia]);
		real taxrate = env->trate[ia];

		for(int ikm=0;ikm<NKM_GRID;ikm++)
		{
			// Interest Rate
			real irate = env->alpha*(env->ag[ia]*pow(env->km[ikm]/env->er[ia]/env->l_bar,env->alpha-1));
			// Wage
			real wage  =(1.-env->alpha)*(env->ag[ia]*pow(env->km[ikm]/env->er[ia]/env->l_bar,env->alpha));

			for(int iid=0;iid<NSTATES_ID;iid++)
				for(int ik=0;ik<NKGRID;ik++)
				{
					env->wealth[IXV(ia, ikm, iid, ik)] = (irate + 1.-env->delta) * env->k[ik] +
					wage * ( (1.-taxrate)* env->epsilon[iid]*env->l_bar+
							(1.-env->epsilon[iid]) * env->mu
							);
					vars->kprime_a[IXV(ia, ikm, iid, ik)]=NKPRIME_SCALE*env->k[ik];
				}
		}
	}
}

void init_k(env_t *env, vars_t *vars)
{
	real *x = (real *) malloc(NKGRID*sizeof(real));
	real *y = (real *) malloc(NKGRID*sizeof(real));

	real xmin = 0;
	real xmax = 0.5;

	linear_space(xmin, xmax, NKGRID, x);
	for(int i=0;i<NKGRID;i++) {
		y[i] = pow(x[i],POWER_POLYNOMIAL_KGRID)/pow(x[NKGRID-1],POWER_POLYNOMIAL_KGRID);
		env->k[i] = KMIN+(KMAX-KMIN)*y[i];
		// env->k_egm[i] = env->k[i];
	}

	linear_space(KM_MIN, KM_MAX, NKM_GRID,env->km);

	free(x);
	free(y);
	return;
}

void init_post (env_t *env)
{
	for (int i = 0; i < NKM_GRID; ++i)
		env->log_env_km[i] = log(env->km[i]);
	for (int i = 0; i < NSTATES_AG; ++i)
		env->irate_factor[i] = env->alpha * env->ag[i];
	for (int i = 0; i < NSTATES_AG; ++i)
		env->wage_factor[i] = (1. - env->alpha) * env->ag[i];

	int cidx = 0;
	for (int i = 0; i < NSTATES_AG; ++i)
		for (int j = 0; j < NSTATES_ID; ++j)
			env->cons2_factor[cidx++] = ((1.-env->trate[i]) 
										* env->epsilon[j]
										* env->l_bar
										+ (1. - env->epsilon[j])
										* env->mu);
}

void init_all(env_t *env, input_t *in, vars_t *vars)
{
	// @@ todo distinguish size in array nelem from bytes
	in->idshock = (shock_t *) malloc(IDSHOCK_ARR_SIZE);
	in->agshock = (unsigned char *) malloc(AGSHOCK_ARR_SIZE);
	// vars->kprime_a  = (real *) malloc(NSTATES*sizeof(real));
	// vars->kprime_b = (real *) malloc(NSTATES*sizeof(real));
	// vars->kcross = (real *) malloc(N_AGENTS*sizeof(real));
	// vars->kcross_l = (real *) malloc(N_AGENTS*sizeof(real));
	// vars->kcross_n = (real *) malloc(N_AGENTS*sizeof(real));
	// vars->coeff = (real *) malloc(NCOEFF*sizeof(real));
	// vars->coeff1 = (real *) malloc(NCOEFF*sizeof(real));
	// vars->kmprime = (real *) malloc(NSTATES_AG*NKM_GRID*sizeof(real));
	// vars->kmts  = (real *) malloc(SIM_STEPS*sizeof(real));

	init_environment(env);

	init_transition(env);

	init_binary_inputs(in);

	init_parmshocks(env);

	init_k(env, vars);

	init_prealloc(env, vars);

	init_post(env);

	vars->coeff[0]=0;
	vars->coeff[1]=1;
	vars->coeff[2]=0;
	vars->coeff[3]=1;

	return;
}

void free_all(input_t *in)
{
	free(in->idshock);
	free(in->agshock);
	return;
}
