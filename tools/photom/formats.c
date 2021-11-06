#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "strops.h"
#include "misc.h"
#include "fits.h"
#include "photom.h"

extern void heliocorr (double jd, double ra, double dec, double *hcp);

#define	MJDBIAS	2449000.0 

/* print the column headings for the standard stuff (no verbose stuff)
 */
void
printHeader()
{
	int i;

	printf("  N File      Z      dX   dY  HC       MHJD       Sky  SkyRMS");
	for (i = 0; i < nstars; i++) {
	    printf ("  V%-5d Err%-2d", i, i);
	    if (wantfwhm)
		printf (" FWM%-2d FWR%-2d", i, i);
	    if (wantxy)
		printf ("  X%-2d  Y%-2d", i, i);
	}
	printf ("\n");
}

void
printStarStats (ssp)
StarStats *ssp;
{
	printf ("p=%5d x=%3d y=%3d Src=%7d rmsSrc=%6.2f rAp=%2d Sky=%5d rmsSky=%5.2f Gauss: x=%7.1f y=%7.1f xfwmh=%6.2f yfwhm=%6.2f\n",
		    ssp->p, ssp->bx, ssp->by, ssp->Src, ssp->rmsSrc,
		    ssp->rAp, ssp->Sky, ssp->rmsSky,
		    ssp->x, ssp->y, ssp->xfwhm, ssp->yfwhm);
}

/* print the image header info for the given image.
 * N.B. we use files[n].ss[nstars-1]
 */
void
printImageHeader (n, fip)
int n;
FImage *fip;
{
	double jd, mhjd;
	double hc;	/* heliocentric time correction */
	double z;	/* airmass */
	char buf[128];
	char rbuf[128], dbuf[128];
	char fn[64], *dp;
	StarStats *refss = &files[n].ss[nstars-1];

	if (getRealFITS (fip, "JD", &jd) < 0)
	    jd = 0.0;

	if (!getStringFITS(fip, "RA", rbuf) && !getStringFITS(fip, "DEC",dbuf)){
	    double ra, dec;
	    double mjd2000;

	    scansex (rbuf, &ra);
	    ra = hrrad (ra);
	    scansex (dbuf, &dec);
	    dec = degrad (dec);

	    year_mjd (2000.0, &mjd2000);
	    precess (jd-MJD0, mjd2000, &ra, &dec);
	    heliocorr (jd, ra, dec, &hc);
	} else
	    hc = 0.0;

	if (!getStringFITS (fip, "ELEVATIO", buf)) {
	    double alt;
	    scansex (buf, &alt);
	    alt = degrad (alt);
	    z = 1.0/sin(alt);
	} else
	    z = 0.0;

	mhjd = jd-MJDBIAS - hc;

	/* remove the root and extension */
	strcpy (fn, basenm(files[n].fn));
	dp = strchr (fn, '.');
	if (dp) *dp = '\0';

	printf ("%3d %-9s %4.2f %4d %4d %8.5f %9.5f %5.0f %6.1f", n, fn, z,
				files[n].dx, files[n].dy, hc, mhjd,
				refss->Sky-files[n].pixdc0, refss->rmsSky);
}

/* print mag info about ss1 wrt ss0 */
void
printStarReport (ss0, ss1)
StarStats *ss0, *ss1;
{
	double m, dm;

	if (starMag (ss0, ss1, &m, &dm) < 0)
	    dm = 9.99;
	if (m > 99.99) m = 99.99;
	if (m < -9.99) m = -9.99;
	if (dm > 9.99) dm = 9.99;
	if (dm < -.99) dm = -.99;

	printf (" %6.3f %6.3f", m, dm);
	if (wantfwhm) {
	    double fwhmm = sqrt (ss1->xfwhm * ss1->yfwhm);
	    double fwhmr = ss1->yfwhm / ss1->xfwhm;
	    printf (" %5.2f %5.2f", fwhmm, fwhmr);
	}
	if (wantxy)
	    printf (" %4d %4d", ss1->bx, ss1->by);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: formats.c,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $"};
