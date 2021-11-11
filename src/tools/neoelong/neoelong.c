/* This program computes the solar elongation angle of a Near Earth Object,
 * given information in the header fields of one or more FITS files and
 * the geocentric distance to the object as a command line argument.
 * The computation centers on finding the geocentric position from the
 * topocentric place. This is based on horizontal parallax as follows:
 *
 *   sin P = (R/d)*cos(h)
 * where
 *     P = horizontal parallax
 *     R = radius of Earth
 *     d = geocentric distance to NEO
 *     h = topocentric altitude
 *
 * Then the geocentric altitude is found simply as
 *   h' = h - P
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"

static void usage (void);
static int oneFITS (char *name);
static int getFields (char *fn, Now *np, double *altp, double *azp);
static double hparallax (double d, double alt);

static int pflag;			/* add FITS filename prefix */
static int vflag;			/* verbose */
static char *pname;			/* me */
static double neod;			/* distance to NEO, km */

int
main (int ac, char *av[])
{
	int ok;

	pname = av[0];

	while ((--ac > 0) && ((*++av)[0] == '-')) {
	    char *s;
	    for (s = av[0]+1; *s != '\0'; s++)
		switch (*s) {
		case 'p':
		    pflag++;
		    break;
		case 'v':
		    vflag++;
		    break;
		default:
		    usage();
		}
	}

	/* first required arg is NEO distance */
	if (ac < 1)
	    usage();
	neod = atof (*av++);
	ac--;

	/* ac remaining args starting at av[0] */
	ok = 0;
	while (ac--)
	    ok += oneFITS (*av++);

	return (-ok);
}

static void
usage()
{
	fprintf (stderr, "Usage: %s [options] d *.fts\n", pname);
	fprintf (stderr, "%s\n", &"$Revision: 1.1.1.1 $"[1]);
	fprintf (stderr, "Purpose: compute solar elongation of NEO objects in FITS files\n");
	fprintf (stderr, "Optional arguments:\n");
	fprintf (stderr, "  -p: prefix each result with name of FITS file\n");
	fprintf (stderr, "  -v: verbose messages to stderr\n");
	fprintf (stderr, "Required arguments:\n");
	fprintf (stderr, "   d: geocentric distance to NEO, km\n");
	fprintf (stderr, "  All additional arguments are FITS files\n");
	fprintf (stderr, "Output format:\n");
	fprintf (stderr, "  One line per FITS file, showing spherical angle between\n");
	fprintf (stderr, "  center of FITS frame and Sun, rads.\n");
	fprintf (stderr, "Exit value:\n");
	fprintf (stderr, "  number of files which had problems\n");

	exit (1);
}

/* process one FITS file.
 * return 0 if ok, else -1
 */
static int
oneFITS (char *name)
{
	Now now, *np = &now;
	Obj o, *op = &o;
	double alt, az;
	double tha, tra, tdec;
	double lst, hpar, e;

	/* get fields for target */
	if (vflag)
	    fprintf (stderr, "%s:\n", name);
	if (getFields (name, np, &alt, &az) < 0)
	    return (-1);

	/* find geocentric target position */
	if (vflag)
	    fprintf (stderr, "  Target at Alt %g Az %g\n", alt, az);
	unrefract (pressure, temp, alt, &alt);
	hpar = hparallax (neod, alt);
	alt += hpar;				/* find geocentric altitude */
	if (vflag)
	    fprintf (stderr, "  hp %g\n", hpar);
	aa_hadec (lat, alt, az, &tha, &tdec);
	now_lst (np, &lst);
	lst = hrrad(lst);
	tra = lst - tha;
	if (vflag)
	    fprintf (stderr, "  Target at RA %g Dec %g\n", tra, tdec);

	/* find geocentric Sun position */
	op->o_type = PLANET;
	op->pl.pl_code = SUN;
	(void) strcpy (op->o_name, "Sun");
	obj_cir (np, op);
	if (vflag)
	    fprintf (stderr, "  Sun at RA %g Dec %g\n", op->s_ra, op->s_dec);

	/* find angular separation */
	solve_sphere (op->s_ra-tra, PI/2-op->s_dec, sin(tdec), cos(tdec),
								    &e, NULL);
	if (pflag)
	    printf ("%12s ", name);
	printf ("%7.5f\n", acos(e));
	return (0);
}

static int
getFields (char *fn, Now *np, double *altp, double *azp)
{
	FImage fimage, *fip = &fimage;
	char buf[1024];
	double tmp;
	int ok = -1;
	int fd;

	/* read the header */
	fd = open (fn, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    return (-1);
	}
	if (readFITSHeader (fd, fip, buf) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, buf);
	    return (-1);
	}
	(void) close (fd);

	/* extract the fields for Now */
	if (getRealFITS (fip, "JD", &tmp) < 0) {
	    fprintf (stderr, "%s: no JD\n", fn);
	    goto bad;
	}
	mjd = tmp - MJD0;

	if (getStringFITS (fip, "LATITUDE", buf) < 0) {
	    fprintf (stderr, "%s: no LATITUDE\n", fn);
	    goto bad;
	}
	if (scansex (buf, &tmp) < 0) {
	    fprintf (stderr, "%s: bad LATITUDE: %s\n", fn, buf);
	    goto bad;
	}
	lat = degrad(tmp);

	if (getStringFITS (fip, "LONGITUD", buf) < 0) {
	    fprintf (stderr, "%s: no LONGITUD\n", fn);
	    goto bad;
	}
	if (scansex (buf, &tmp) < 0) {
	    fprintf (stderr, "%s: bad LONGITUD: %s\n", fn, buf);
	    goto bad;
	}
	lng = degrad(tmp);

	tz = 0;

	if (getRealFITS (fip, "WXTEMP", &temp) < 0) {
	    fprintf (stderr, "%s: no WXTEMP\n", fn);
	    goto bad;
	}

	if (getRealFITS (fip, "WXPRES", &pressure) < 0) {
	    fprintf (stderr, "%s: no WXPRES\n", fn);
	    goto bad;
	}

	elev = 0;

	dip = 0;

	epoch = EOD;

	tznm[0] = '\0';

	/* get alt/az */

	if (getStringFITS (fip, "ELEVATIO", buf) < 0) {
	    fprintf (stderr, "%s: no ELEVATIO\n", fn);
	    goto bad;
	}
	if (scansex (buf, &tmp) < 0) {
	    fprintf (stderr, "%s: bad ELEVATIO: %s\n", fn, buf);
	    goto bad;
	}
	*altp = degrad(tmp);

	if (getStringFITS (fip, "AZIMUTH", buf) < 0) {
	    fprintf (stderr, "%s: no AZIMUTH\n", fn);
	    goto bad;
	}
	if (scansex (buf, &tmp) < 0) {
	    fprintf (stderr, "%s: bad AZIMUTH: %s\n", fn, buf);
	    goto bad;
	}
	*azp = degrad(tmp);

	/* phew! */
	ok = 0;

    bad:
	resetFImage (fip);
	return (ok);
}

/* given NEO distance in km and altitude, return horizontal parallax.
 * both angles in rads.
 */
static double
hparallax (double d, double a)
{
	return (asin((ERAD/1000)/d*cos(a)));
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: neoelong.c,v $ $Date: 2003/04/15 20:48:35 $ $Revision: 1.1.1.1 $ $Name:  $"};
