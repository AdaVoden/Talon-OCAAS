/* given a list of at least two HA/Dec/X/Y compute a best mount model.
 * initial conditions can come from config files or command line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "strops.h"
#include "configfile.h"
#include "telstatshm.h"

static void usage (char *p);
static void initCfg (TelAxes *tap, double *ftolp);
static int readHDXY(double **H, double **D, double **X, double **Y);
static void printTax (TelAxes *tp);

static char hcfn[] = "archive/config/home.cfg";
static char tcfn[] = "archive/config/telescoped.cfg";

static double ftoldef = 1e-6;
static TelAxes tax;
static double ftol;
static int vflag;

int
main (int ac, char *av[])
{
	TelAxes *tp = &tax;
	double *H, *D, *X, *Y, *resid;
	char *pname;
	time_t t;
	char *ds;
	int i, n;

	pname = basenm(av[0]);

	/* crack args */
	while ((--ac > 0) && ((*++av)[0] == '-')) {
	    char *s;
	    for (s = av[0]+1; *s != '\0'; s++)
		switch (*s) {
		case 'D':	/* new DT */
		    if (ac < 2)
			usage (pname);
		    tp->DT = atof(*++av);
		    ac--;
		    break;
		case 'H':	/* new HT */
		    if (ac < 2)
			usage (pname);
		    tp->HT = atof(*++av);
		    ac--;
		    break;
		case 'X':	/* new XP */
		    if (ac < 2)
			usage (pname);
		    tp->XP = atof(*++av);
		    ac--;
		    break;
		case 'Y':	/* new YC */
		    if (ac < 2)
			usage (pname);
		    tp->YC = atof(*++av);
		    ac--;
		    break;
		case 'd':	/* get defaults from config files */
		    initCfg (tp, &ftol);
		    break;
		case 'f':	/* GERMEQ_FLIP */
		    tp->GERMEQ_FLIP = 1;
		    break;
		case 'g':	/* GERMEQ */
		    tp->GERMEQ = 1;
		    break;
		case 't':	/* fit goal */
		    if (ac < 2)
			usage (pname);
		    ftol = atof(*++av);
		    ac--;
		    break;
		case 'v':	/* verbose */
		    vflag++;
		    break;
		case 'z':	/* ZENFLIP */
		    tp->ZENFLIP = 1;
		    break;
		default:
		    usage (pname);
		}
	}

	/* ac remaining args starting at av[0] */
	if (ac > 0)
	    usage (pname);

	/* slurp up stdin */
	n = readHDXY(&H, &D, &X, &Y);
	if (n < 0)
	    exit(1);
	if (n < 2) {
	    fprintf (stderr, "Require 2+ points but only saw %d\n", n);
	    exit(2);
	}

	/* print initial condition if verbose */
	if (vflag)
	    printTax (tp);

	/* solve */
	resid = (double *) calloc (n, sizeof(double));
	if (tel_solve_axes (H, D, X, Y, n, ftol, tp, resid) < 0) {
	    fprintf (stderr, "Solution failed\n");
	    exit (3);
	}

	/* print just like in home.cfg */
	time (&t);
	ds = asctime(gmtime(&t));
	printf ("HT\t\t%10.7f\t! Update UTC %s", tp->HT, ds);
	printf ("DT\t\t%10.7f\t! Update UTC %s", tp->DT, ds);
	printf ("XP\t\t%10.7f\t! Update UTC %s", tp->XP, ds);
	printf ("YC\t\t%10.7f\t! Update UTC %s", tp->YC, ds);
	printf ("NP\t\t%10.7f\t! Update UTC %s", tp->NP, ds);

	/* print all residuals too if desired */
	if (vflag)
	    for (i = 0; i < n; i++)
		fprintf (stderr, "%10.7f %10.7f %10.7f %10.7f %10.7f'\n",
				H[i], D[i], X[i], Y[i], 60*raddeg(resid[i]));

	return (0);
}

static void
usage(char *p)
{
fprintf(stderr, "%s: [options]\n", p);
fprintf(stderr, "Purpose: find best mount model from pointing data.\n");
fprintf(stderr, "Synopsis: pterrors -t *.fts | %s >> home.cfg\n", p);
fprintf(stderr, " -H HT: set initial guess at HT (use after -d to override)\n");
fprintf(stderr, " -D DT: set initial guess at DT (use after -d to override)\n");
fprintf(stderr, " -X XP: set initial guess at XP (use after -d to override)\n");
fprintf(stderr, " -Y YC: set initial guess at YC (use after -d to override)\n");
fprintf(stderr, " -t t : set desired model tolerance, rads (use after -d to override)\n");
fprintf(stderr, " -d   : get all defaults from %s and %s\n",
						basenm(tcfn), basenm(hcfn));
fprintf(stderr, " -f   : turn on GERMEQ_FLIP\n");
fprintf(stderr, " -g   : turn on GERMEQ\n");
fprintf(stderr, " -v   : verbose\n");
fprintf(stderr, " -z   : turn on ZENFLIP\n");

exit (1);
}

/* fill in the tap from home.cfg */
static void
initCfg (TelAxes *tap, double *ftolp)
{
#define NHCFG   (sizeof(hcfg)/sizeof(hcfg[0]))
#define NTCFG   (sizeof(tcfg)/sizeof(tcfg[0]))
 	static double HT, DT, XP, YC;
	static CfgEntry hcfg[] = {
	    {"HT",		CFG_DBL, &HT},
	    {"DT",		CFG_DBL, &DT},
	    {"XP",		CFG_DBL, &XP},
	    {"YC",		CFG_DBL, &YC},
	};
 	static double HESTEP, DESTEP;
	static int GERMEQ, ZENFLIP;
	static CfgEntry tcfg[] = {
	    {"HESTEP",		CFG_DBL, &HESTEP},
	    {"DESTEP",		CFG_DBL, &DESTEP},
	    {"GERMEQ",		CFG_INT, &GERMEQ},
	    {"ZENFLIP",		CFG_INT, &ZENFLIP},
	};
	int n;

	n = readCfgFile (vflag, hcfn, hcfg, NHCFG);
	if (n != NHCFG) {
	    fprintf(stderr,"Erroring getting model defaults.. leaving all 0\n");
	} else {
	    tap->HT = HT;
	    tap->DT = DT;
	    tap->XP = XP;
	    tap->YC = YC;
	}

	n = readCfgFile (vflag, tcfn, tcfg, NTCFG);
	if (n != NTCFG) {
	    fprintf (stderr, "Error getting tolerance default.. using %g\n",
								    ftoldef);
	    *ftolp = ftoldef;
	} else
	    *ftolp  = 1./(HESTEP <= DESTEP ? HESTEP : DESTEP);
}

/* read a file of "H D X Y" off stdin and create malloced arrays.
 * return number of.
 * N.B. caller gets back malloced memory regardless of return value.
 */
static int
readHDXY(double **H, double **D, double **X, double **Y)
{
	double *h, *d, *x, *y;
	char buf[1024];
	int n;

	/* get things started */
	h = (double *) malloc (sizeof(double));
	d = (double *) malloc (sizeof(double));
	x = (double *) malloc (sizeof(double));
	y = (double *) malloc (sizeof(double));

	for (n = 0; fgets (buf, sizeof(buf), stdin); ) {
	    if(sscanf(buf,"%lf %lf %lf %lf", &h[n], &d[n], &x[n], &y[n]) == 4){
		n++;
		h = realloc (h, (n+1)*sizeof(double));
		d = realloc (d, (n+1)*sizeof(double));
		x = realloc (x, (n+1)*sizeof(double));
		y = realloc (y, (n+1)*sizeof(double));
	    }
	}

	*H = h;
	*D = d;
	*X = x;
	*Y = y;
	return (n);
}

static void
printTax (TelAxes *tp)
{
	fprintf (stderr, "HT = %10.7g\n", tp->HT);
	fprintf (stderr, "DT = %10.7g\n", tp->DT);
	fprintf (stderr, "XP = %10.7g\n", tp->XP);
	fprintf (stderr, "YC = %10.7g\n", tp->YC);
	fprintf (stderr, "NP = %10.7g\n", tp->NP);
	fprintf (stderr, "GERMEQ = %d\n", !!tp->GERMEQ);
	fprintf (stderr, "GERMEQ_FLIP = %d\n", !!tp->GERMEQ_FLIP);
	fprintf (stderr, "ZENFLIP = %d\n", !!tp->ZENFLIP);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: mntmodel.c,v $ $Date: 2003/04/15 20:48:35 $ $Revision: 1.1.1.1 $ $Name:  $"};
