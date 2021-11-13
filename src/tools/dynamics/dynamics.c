/* study axis dynamics */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "P_.h"
#include "astro.h"

#define S0	5.	/* initial scope position, degrees */
#define V0	0	/* initial scope velocity, degs/sec */
#define	PER	.000278	/* max allows position error, degrees */
#define	TER	.03	/* max jitter in time sample, secs */

static double starx(double t);
static void usage (char *p);

int
main (int ac, char *av[])
{
	/* t -- time
	 * o -- object
	 * s -- scope
	 */
	double DT;		/* sample interval, seconds */
	double SV;		/* max scope velocity, degs/sec */
	double SA;		/* max scope acceleration, degs/sec/sec */
	double DF;		/* damping factor */
	double t0;		/* current check time */
	double tp;		/* previous time */
	double sp;		/* scope pos @ tp */
	double sv;		/* scope vel */
	int locked = 0;

	if (ac != 5)
	    usage(av[0]);

	DT = atof(av[1])/1000.;
	SV = raddeg(atof(av[2]));
	SA = raddeg(atof(av[3]));
	DF = atof(av[4]);

	tp = 0;
	sp = S0;
	sv = V0;

	for (t0 = DT; t0 < 100; t0 += DT) {
	    double t1;		/* prediction time */
	    double o0;		/* obj pos @ t0 */
	    double o1;		/* obj pos @ t1 */
	    double s0;		/* scope pos @ t0 */
	    double s1;		/* scope pos @ t1 */
	    double ov;		/* obj vel */
	    double iv;		/* ideal scope vel to hit s1 @ t1 */
	    double nv;		/* proposed new scope vel */

	    t0 += TER*(-1.0 + 2.0*random()/RAND_MAX);
	    s0 = sp + sv*(t0-tp);
	    t1 = t0 + DT;
	    o0 = starx (t0);
	    o1 = starx (t1);
	    s1 = s0 + sv+DT;

	    ov = (o1 - o0)/DT;
	    iv = (o1 - s0)/DT;

	    nv = DF*ov + (1-DF)*iv;

	    if (!locked)
		locked = fabs(o0-s0) <= PER;
	    if (nv > sv + DT*SA) sv += DT*SA;
	    else if (nv < sv - DT*SA) sv -= DT*SA;
	    else sv = nv;
	    if (sv > SV) sv = SV;
	    if (sv < -SV) sv = -SV;

	    printf ("%g %g %g %g %g\n", t0, o0, s0, o0-s0, S0*locked);

	    sp = s0;
	    tp = t0;
	}

	return (0);
}

static double
starx(double t)
{
	double tp = t/20;

	return (sin(tp*tp));
}

static void
usage (char *p)
{
#define	FS fprintf (stderr,

FS "%s: predict scope axis dynamical behavior\n", p);
FS "usage: DT MV MA DF\n");
FS "  DT: sample interval, ms\n");
FS "  MV: max velocity, rads/sec\n");
FS "  MA: max acceleration, rads/sec/sec\n");
FS "  DF: damping factor, 0..1\n");
FS "Output: T O S E L\n");
FS "  T : elapsed time, 0..100 seconds\n");
FS "  O : target position, degrees\n");
FS "  S : scope position, degrees\n");
FS "  E : error, O-S\n");
FS "  L : 0 until E <= 1\", then %g\n", S0);
FS "Examples of plotting with GNUPLOT; assumes output saved in file F:\n");
FS "  Set up plotting with fine lines:\n");
FS "    gnuplot> set data style lines\n");
FS "  Plot entire run, and mark time of tracking lock:\n");
FS "    gnuplot> plot 'F' using 1:2, 'F' using 1:3, 'F' using 1:5\n");
FS "  Plot errors well after lock:\n");
FS "    gnuplot> plot [30:] 'F' using 1:4\n");

	exit(1);
}
