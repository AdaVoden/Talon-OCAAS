#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>

#include "optim.h"

/* least squares solver.
 * returns number of iterations if solution converged, else -1.
 */
int
lstsqr (
double (*chisqr)(double p[]),	/* function to evaluate chisqr with at p */
double params0[],		/* in: guess: back: best */
double params1[],		/* second guess to set characteristic scale */
int np,				/* entries in params0[] and params1[] */
double ftol)			/* desired fractional tolerance */
{
	int numVars = np;
	double Xtol = ftol;
	int maxIter = 100;
	Vector Xarr;
	int i, s;

	newVect (&Xarr, numVars);
	for (i = 0; i < numVars; i++)
	    VEC (Xarr, i) = params0[i];

	s = NewtonMultiMin (Xarr, numVars, Xtol, maxIter, chisqr);

	if (s)
	    for (i = 0; i < numVars; i++)
		params0[i] = VEC (Xarr, i);

	deleteVect (&Xarr);

	return (s ? 0 : -1);
}
