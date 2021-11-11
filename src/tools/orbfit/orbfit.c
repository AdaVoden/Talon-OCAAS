/* given three observation, find heliocentric orbit elements */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "lstsqr.h"
#include "strops.h"
#include "configfile.h"

#define	COMMENT	'#'

typedef struct {
    double ra, dec;	/* J2000 coordinates */
    double Mjd;		/* time, as an mjd */
} Observation;

static Observation *obs;	/* malloced list of observations */
static int nobs;		/* number of observations */
static Obj obj;
static Now now;
static double chisqr;

static void usage(char *prognam);
static void readFile (char *fn);
static void read1 (char *fn, FILE *fp, char buf[], int l);
static int read1noex (char *fn, FILE *fp, char buf[], int l);
static void crack1 (char fn[], char line[], double *dp);
static void crackmjd (char fn[], char line[], double *mjdp);
static void crackObs (char fn[], char line[], Observation *op);
static int findPOrbit (void);
static int findEOrbit (void);

static int gflag;	/* geocentric, not topocentric using telsched.cfg */
static int vflag;	/* verbose */
static int lineno;	/* running line number for err message */

int
main (int ac, char *av[])
{
	char *progname = av[0];
	int n;

	/* crack arguments */
	for (av++; --ac > 0 && *(*av) == '-'; av++) {
	    char c;
	    while ((c = *++(*av)))
		switch (c) {
		case 'g':
		    gflag++;
		    break;
		case 'v':
		    vflag++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining file names starting at av[0] */
	if (ac == 0) {
	    fprintf (stderr, "No config file.\n");
	    usage (progname);
	}
	if (ac > 1) {
	    fprintf (stderr, "Extra files.\n");
	    usage (progname);
	}

	readFile (av[0]);
	if (nobs < 3) {
	    fprintf (stderr, "Require at least 3 observations\n");
	    exit (1);
	}
	if (obj.o_type == PARABOLIC)
	    n = findPOrbit ();
	else
	    n = findEOrbit ();

	if (n < 0)
	    fprintf (stderr, "No solution\n");
	else {
	    char buf[1024];
	    printf ("Chi=%g degrees\n", raddeg(sqrt(chisqr)));
	    db_write_line (&obj, buf);
	    printf ("%s\n", buf);
	}

	return (n < 0 ? 1 : 0);
}

static void
usage(char *progname)
{
#define	FPF fprintf (stderr,

FPF "%s: Usage: [options] config_file\n", basenm(progname));
FPF "Purpose: given three J2000 observations, find heliocentric orbital elements.\n");
FPF "Options:\n");
FPF "  -g: geocentric, default is topocentric based on LAT/LONG in telsched.cfg.\n");
FPF "  -v: verbose\n");
FPF "Config file format for elliptical solution is 11 lines as follows:\n");
FPF "  `Elliptical' keyword (without the quotes)\n");
FPF "  guess for inclination, degrees\n");
FPF "  guess for longitude of ascending node, degrees\n");
FPF "  guess for argument of perihelion, degrees\n");
FPF "  guess for mean distance (aka semi-major axis), AU\n");
FPF "  guess for eccentricity\n");
FPF "  guess for mean anomaly (i.e., degrees from perihelion)\n");
FPF "  guess for epoch date, MM/DD.D/YYYY (i.e., time of mean anomaly)\n");
FPF "Config file format for parabolic solution is 9 lines as follows:\n");
FPF "  `Parabolic' keyword (without the quotes)\n");
FPF "  guess for epoch of perihelion, MM/DD.D/YYYY\n");
FPF "  guess for inclination, degrees\n");
FPF "  guess for perihelion distance, AU\n");
FPF "  guess for argument of perihelion, degrees\n");
FPF "  guess for longitude of ascending node, degrees\n");
FPF "All remaining lines of either format are the observations as follows:\n");
FPF "  JD  HH:MM:SS.S  DD:MM:SS\n"); 
FPF "  JD  HH:MM:SS.S  DD:MM:SS\n"); 
FPF "  JD  HH:MM:SS.S  DD:MM:SS\n"); 
FPF "  ...\n");

#undef FPF

	exit (1);
}

static void
readFile (char *fn)
{
	Obj *op = &obj;
	char buf[1024];
	double x;
	FILE *fp;

	/* open the file */
	fp = fopen (fn, "r");
	if (!fp) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    exit (1);
	}

	/* init the Obj */
	memset ((void *)op, 0, sizeof(*op));

	/* first line determines remaining format */
	read1 (fn, fp, buf, sizeof(buf));
	if (strcwcmp (buf, "Elliptical") == 0) {
	    strcpy (op->o_name, "Asteroid X");
	    op->o_type = ELLIPTICAL;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->e_inc = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->e_Om = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->e_om = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->e_a = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->e_e = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->e_M = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crackmjd (fn, buf, &x);
	    op->e_cepoch = x;

	    op->e_epoch = J2000;

	} else if (strcwcmp (buf, "Parabolic") == 0) {
	    strcpy (op->o_name, "Comet X");
	    op->o_type = PARABOLIC;

	    op->p_epoch = J2000;

	    read1 (fn, fp, buf, sizeof(buf));
	    crackmjd (fn, buf, &x);
	    op->p_ep = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->p_inc = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->p_qp = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->p_om = x;

	    read1 (fn, fp, buf, sizeof(buf));
	    crack1 (fn, buf, &x);
	    op->p_Om = x;

	} else {
	    fprintf (stderr, "%s: unknown leading type keyword\n", fn);
	    exit (1);
	}

	/* remaining lines are observations */
	obs = (Observation *) malloc (1);	/* so we can always realloc */
	for (nobs = 0; read1noex (fn, fp, buf, sizeof(buf)) == 0; nobs++) {
	    obs = (Observation *) realloc (obs, (nobs+1)*sizeof(Observation));
	    crackObs (fn, buf, &obs[nobs]);
	}

	/* finished with file */
	(void) fclose (fp);
}

static double
parabolic_chisqr (double p[5])
{
	Now *np = &now;
	Obj *op = &obj;
	double cs = 0;
	int i;

	op->p_ep = p[0];
	op->p_inc = p[1];
	op->p_qp = p[2];
	op->p_Om = p[3];
	op->p_om = p[4];

	epoch = J2000;

	for (i = 0; i < nobs; i++) {
	    Observation *bp = &obs[i];
	    double c, ca;

	    mjd = bp->Mjd;
	    (void) obj_cir (np, op);
	    solve_sphere (bp->ra - op->s_ra, PI/2-bp->dec, sin(op->s_dec),
						    cos(op->s_dec), &ca, NULL);
	    c = acos (ca);
	    cs += c*c;
	}

	if (vflag > 1)
	    fprintf (stderr, "%7.3f: %7.3f,%7.3f,%7.3f,%7.3f,%7.3f\n",
		cs, op->p_ep, op->p_inc, op->p_qp, op->p_Om, op->p_om);

	chisqr = cs;
	return (cs);
}

static double
elliptical_chisqr (double p[6])
{
	Now *np = &now;
	Obj *op = &obj;
	double cs = 0;
	int i;

	op->e_inc = p[0];
	op->e_Om = p[1];
	op->e_om = p[2];
	op->e_a = p[3];
	op->e_e = p[4];
	op->e_M = p[5];

	epoch = J2000;

	for (i = 0; i < nobs; i++) {
	    Observation *bp = &obs[i];
	    double c, ca;

	    mjd = bp->Mjd;
	    (void) obj_cir (np, op);
	    solve_sphere (bp->ra - op->s_ra, PI/2-bp->dec, sin(op->s_dec),
						    cos(op->s_dec), &ca, NULL);
	    c = acos (ca);
	    cs += c*c;
	}

	if (vflag > 1)
	    fprintf (stderr,"%9.6f: %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f\n",
		cs, op->e_inc, op->e_Om, op->e_om, op->e_a, op->e_e, op->e_M);

	chisqr = cs;
	return (cs);
}

/* solve for obj from obs.
 * return -1 if trouble, else number of iterations.
 */
static int
findPOrbit ()
{
	Obj *op = &obj;
	double p0[5], p1[5];
	int s;

	p0[0] = op->p_ep;
	p0[1] = op->p_inc;
	p0[2] = op->p_qp;
	p0[3] = op->p_Om;
	p0[4] = op->p_om;

	p1[0] = 0.9 * (p0[4] + .1);
	p1[1] = 0.9 * (p0[0] + .1);
	p1[2] = 0.9 * (p0[3] + .1);
	p1[3] = 0.9 * (p0[1] + .1);
	p1[4] = 0.9 * (p0[2] + .1);

	s = lstsqr (parabolic_chisqr, p0, p1, 5, .0001);

	if (s > 0) {
	    op->p_ep = p0[0];
	    op->p_inc = p0[1];
	    op->p_qp = p0[2];
	    op->p_Om = p0[3];
	    op->p_om = p0[4];
	}

	return (s);
}

/* solve for obj from obs.
 * return -1 if trouble, else number of iterations.
 */
static int
findEOrbit ()
{
	Obj *op = &obj;
	double p0[6], p1[6];
	int s;

	p0[0] = op->e_inc;
	p0[1] = op->e_Om;
	p0[2] = op->e_om;
	p0[3] = op->e_a;
	p0[4] = op->e_e;
	p0[5] = op->e_M;

	p1[0] = 0.9 * (p0[0] + 10);
	p1[1] = 0.9 * (p0[1] + 10);
	p1[2] = 0.9 * (p0[2] + 10);
	p1[3] = 0.9 * (p0[3] + .2);
	p1[4] = 0.9 * (p0[4] + 0.1);
	p1[5] = 0.9 * (p0[5] + 10);

	s = lstsqr (elliptical_chisqr, p0, p1, 6, .0001);

	if (s >= 0) {
	    op->e_inc = p0[0];
	    op->e_Om = p0[1];
	    op->e_om = p0[2];
	    op->e_a = p0[3];
	    op->e_e = p0[4];
	    op->e_M = p0[5];
	}

	return (s);
}

static void
read1 (char *fn, FILE *fp, char buf[], int l)
{
	if (read1noex (fn, fp, buf, l) < 0) {
	    fprintf (stderr, "%s: file is short\n", fn);
	    exit (1);
	}
}

/* return 0 if find another line, else -1 */
static int
read1noex (char *fn, FILE *fp, char buf[], int l)
{
	while (fgets (buf, l, fp)) {
	    lineno++;
	    if (buf[0] != COMMENT && buf[0] != '\n')
		return (0);
	}

	return (-1);
}

static void
crack1 (char fn[], char line[], double *dp)
{
	if (sscanf (line, "%lf", dp) != 1) {
	    fprintf (stderr, "%s: bad number format on line %d\n", fn, lineno);
	    exit (1);
	}
}

static void
crackmjd (char fn[], char line[], double *mjdp)
{
	double d;
	int m, y;

	if (sscanf (line, "%d/%lf/%d", &m, &d, &y) != 3) {
	    fprintf (stderr, "%s: bad epoch format on line %d\n", fn, lineno);
	    exit (1);
	}
	cal_mjd (m, d, y, mjdp);
}

static void
crackObs (char fn[], char line[], Observation *op)
{
	double rh, rm, rs;
	double dd, dm, ds;
	double jd;
	char dsign;

	if (sscanf (line, "%lf %lf:%lf:%lf %c%lf:%lf:%lf",
		    &jd, &rh, &rm, &rs, &dsign, &dd, &dm, &ds) != 8) {
	    fprintf (stderr, "%s: bad observation line %d\n", fn, lineno);
	    exit (1);
	}

	op->ra = hrrad(rh + rm/60.0 + rs/3600.0);
	op->dec = degrad(dd + dm/60.0 + ds/3600.0);
	if (dsign == '-')
	    op->dec *= -1;
	op->Mjd = jd - MJD0;
}
