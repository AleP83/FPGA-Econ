#include "init.h"
#include "definitions.h"

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

void init_grids(env_t *env)
{
	linear_space(KMIN, KMAX, J,env->k);
	return;
}

void init_all(env_t *env)
{
	// @@ todo distinguish size in array nelem from bytes
	init_grids(env);
	return;
}

