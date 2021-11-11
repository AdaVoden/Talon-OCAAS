#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "misc.h"


extern int showall;

#define	SUNDOWN	0.2		/* rads sun is below horizon we consider dark */
#define	MINALT	degrad(15.0)	/* min allowable altitude */
#define	MAXHA	hrrad(6)	/* max abs(HA), rads */

static char hah[] = "  ?  ";

static int report (double mjdx, char *name, double ra, double dec);

/* find and print info about the next nminima eclipses of the given object
 * starting at mjdstart.
 * mjdstart is heliocentric, but we report geocentric.
 */
void
eclipses (
double mjdstart,	/* starting date for search */
int nminima,		/* number of minima to find */
char *name,		/* name of star */
double ra,		/* J2000 RA, rads */
double dec,		/* J2000 Dec, rads */
double p,		/* period, days */
double mjd0		/* mjd of one minima */
)
{
	double t = mjdstart;
	int n;

	/* advance mjdstart to first eclipse there after */
	t = mjd0 + p*ceil ((mjdstart - mjd0)/p);

	/* search for the nminima */
	for (n = 0; n < nminima; ) {
	    if (report (t, name, ra, dec) == 0)
		n++;
	    t += p;
	}
}

static int
report (double mjdx, char *name, double ra, double dec)
{
	static Now now;
	static int nowok;
	double dawn, dusk;
	double alt;
	int status;
	double hcp;
	int m, y;
	double d;
	char duskstr[16], utcstr[16], dawnstr[16];
	char lststr[16], hastr[16], altstr[16];
	double l, ha;
	Obj o;

	if (!nowok) {
	    char *latenv = getenv ("LATITUDE");
	    char *lngenv = getenv ("LONGITUDE");

	    if (!latenv) {
		fprintf (stderr,
			"Require LATITUDE environment variable: rads +N\n");
		exit (1);
	    }
	    if (!lngenv) {
		fprintf (stderr,
			"Require LONGITUDE environment variable: rads +W\n");
		exit (1);
	    }
	    now.n_lat = atof (latenv);
	    now.n_lng = -atof (lngenv);

	    now.n_temp = 25.0;
	    now.n_pressure = 1000.0;
	    now.n_elev = 0.0;

	    nowok = 1;

	    /* print the header while we're at it */
	    printf ("Name       MM/DD/YY  JD            Dusk  UT    Dawn  LST   HA     Elev\n");
	}

	/* find geocentric time.
	 * N.B. this neglects earth's motion for the last 8 minutes
	 */
	heliocorr (mjdx+MJD0, ra, dec, &hcp);
	mjdx += hcp;

	/* get info about the object now */
	now.n_mjd = mjdx;
	o.o_type = FIXED;
	o.f_RA = (float)ra;
	o.f_dec = (float)dec;
	o.f_epoch = (float)J2000;
	obj_cir (&now, &o);
	alt = o.s_alt;
	now_lst (&now, &l);
	l = hrrad(l);
	ha = l - ra;
	haRange (&ha);

	if (!showall) {
	    /* check for being up at all */
	    if (alt < MINALT)
		return (-1);

	    /* check for being in HA range */
	    if (fabs(ha) > MAXHA)
		return (-1);

	    /* check for being between dusk and dawn */
	    o.o_type = PLANET;
	    o.pl.pl_code = SUN;
	    obj_cir (&now, &o);
	    if (o.s_alt > -SUNDOWN)
		return (-1);
	}

	twilight_cir (&now, SUNDOWN, &dawn, &dusk, &status);
	if (status & (RS_NORISE|RS_CIRCUMPOLAR|RS_NEVERUP|RS_ERROR))
	    strcpy (dawnstr, hah);
	else
	    fs_sexa (dawnstr, mjd_hr(dawn), 2, 60);
	if (status & (RS_NOSET|RS_CIRCUMPOLAR|RS_NEVERUP|RS_ERROR))
	    strcpy (duskstr, hah);
	else
	    fs_sexa (duskstr, mjd_hr(dusk), 2, 60);

	/* the report */
	mjd_cal (mjdx, &m, &d, &y);
	fs_sexa (utcstr, mjd_hr(mjdx), 2, 60);
	fs_sexa (lststr, radhr(l), 2, 60);
	fs_sexa (hastr, radhr(ha), 3, 60);
	fs_sexa (altstr, raddeg(alt), 2, 60);
	printf ("%-10s %2d/%02d/%2d %14.5f %s %s %s %s %s %s\n",
			    name, m, (int)d, y-1900, mjdx+MJD0,
			    duskstr, utcstr, dawnstr, lststr, hastr, altstr);
	/* ok */
	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: eclipses.c,v $ $Date: 2003/04/15 20:48:38 $ $Revision: 1.1.1.1 $ $Name:  $"};
