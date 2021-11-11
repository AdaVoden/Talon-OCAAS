/* New version of tools/findstars/findstars.c using higher order astrometric
 * fit parameters AMDX*, AMDY* instead of WCS parameters C*
 *
 * Last update DJA 020917
 * Merged STO 021001
 *
 * New features enabled by defining USE_DISTANCE_METHOD as 1 in wcs.h
 */

/* given a file, print location and bightest pixel of all stars.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "strops.h"
#include "wcs.h"

static void usage (char *p);
static void findem (char *fn);
#if USE_DISTANCE_METHOD
static int getAMDFITS (FImage *fip, double a[], double b[]);
static void xy2xieta
    (double a[], double b[], double x, double y, double *xi, double *eta);
static void xieta2RADec (double rc, double dc, int n, double *xi, double *eta,
    double *r, double *d);
#endif
static char *progname;
static int rflag;
static int hflag;

int
main (int ac, char *av[])
{
	char *str;
	char oldIpCfg[256];

	// Save existing IP.CFG.... be sure to restore before any exit!!	
	strcpy(oldIpCfg,getCurrentIpCfgPath());

	/* crack arguments */
	progname = basenm(av[0]);
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str)) {
		switch (c) {
		case 'r':
		    rflag++;
		    break;
		case 'h':
		    hflag++;
		    break;
		case 'i':	/* alternate ip.cfg */
			if(ac < 2) {
				fprintf(stderr, "-i requires ip.cfg pathname\n");
				setIpCfgPath(oldIpCfg);
				usage(progname);
			}
			setIpCfgPath(*++av);
			ac--;
			break;
		default:
			setIpCfgPath(oldIpCfg);
		    usage (progname);
		    break;
		}
	    }
	}

	if (ac == 0) {
		setIpCfgPath(oldIpCfg);
	    usage (progname);
	}
	
	while (ac-- > 0)
	    findem (*av++);

	setIpCfgPath(oldIpCfg);	
	return (0);
}

static void
usage (char *p)
{
	fprintf (stderr, "Usage: %s [options] *.fts\n", p);
	fprintf (stderr, "Purpose: list star-like objects in FITS files\n");
	fprintf (stderr, "Options:\n");
	fprintf (stderr, " -r: report in rads -- default is H:M:S D:M:S\n");
	fprintf (stderr, " -h: high prec format: Alt Az RA Dec M HW VW JD dt\n");
	fprintf (stderr, " -i path: specify alternate ip.cfg location\n");
	exit (1);
}

static void
findem (char *fn)
{
	FImage fimage, *fip = &fimage;
	Now now, *np = &now;
	double eq = 0;
	double lst = 0;
	double expt = 0;
	char buf[1024];
	StarStats *ssp;
	int i, ns, fd;
#if USE_DISTANCE_METHOD
    double a[14], b[14];    /* params for higher order astrometric fit */
#endif
	/* open and read the image */
	fd = open (fn, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    return;
	}
	initFImage (fip);
	i = readFITS (fd, fip, buf);
	(void) close (fd);
	if (i < 0) {
	    fprintf (stderr, "%s: %s\n", fn, buf);
	    resetFImage (fip);
	    return;
	}

	/* gather local info if want alt/az */
	if (hflag) {
	    double tmp;

	    memset (np, 0, sizeof(Now));

	    if (getRealFITS (fip, "JD", &tmp) < 0) {
		fprintf (stderr, "%s: no JD\n", fn);
		resetFImage (fip);
		return;
	    }
	    mjd = tmp - MJD0;

	    if (getStringFITS (fip, "LATITUDE", buf) < 0) {
		fprintf (stderr, "%s: no LATITUDE\n", fn);
		return;
	    }
	    if (scansex (buf, &tmp) < 0) {
		fprintf (stderr, "%s: badly formed LATITUDE: %s\n", fn, buf);
		return;
	    }
	    lat = degrad(tmp);

	    if (getStringFITS (fip, "LONGITUD", buf) < 0) {
		fprintf (stderr, "%s: no LONGITUD\n", fn);
		return;
	    }
	    if (scansex (buf, &tmp) < 0) {
		fprintf (stderr, "%s: badly formed LONGITUD: %s\n", fn, buf);
		return;
	    }
	    lng = degrad(tmp);

	    temp = getStringFITS (fip, "WXTEMP", buf) ? 10 : atof(buf);
	    pressure = getStringFITS (fip, "WXPRES", buf) ? 1010 : atof(buf);

	    elev = 0;	/* ?? */

	    if (getRealFITS (fip, "EQUINOX", &eq) < 0
				    && getRealFITS (fip, "EPOCH", &eq) < 0) {
		fprintf (stderr, "%s: no EQUINOX or EPOCH\n", fn);
		resetFImage (fip);
		return;
	    }
	    year_mjd (eq, &eq);

	    if (getStringFITS (fip, "LST", buf) < 0) {
		fprintf (stderr, "%s: no LST\n", fn);
		return;
	    }
	    if (scansex (buf, &tmp) < 0) {
		fprintf (stderr, "%s: badly formed LST: %s\n", fn, buf);
		return;
	    }
	    lst = hrrad(tmp);

	    if (getRealFITS (fip, "EXPTIME", &expt) < 0) {
		fprintf (stderr, "%s: no EXPTIME\n", fn);
		resetFImage (fip);
		return;
	    }
	}
#if USE_DISTANCE_METHOD
	if (getAMDFITS (fip, a, b) < 0) {
	  fprintf (stderr, "%s:  missing AMD* fields\n", fn);
	  return;
	}
#endif
	ns = findStatStars (fip->image, fip->sw, fip->sh, &ssp);
	for (i = 0; i < ns; i++) {
	    StarStats *sp = &ssp[i];
	    double ra, dec;
#if USE_DISTANCE_METHOD
	    double xi, eta;

	    xy2xieta (a, b, sp->x, sp->y, &xi, &eta);
	    xieta2RADec (a[0], b[0], 1, &xi, &eta, &ra, &dec);
#else	
	    if (xy2RADec (fip, sp->x, sp->y, &ra, &dec) < 0) {
		fprintf (stderr, "%s: no WCS\n", fn);
		break;
	    }
#endif
	    if (rflag)
		printf ("%9.6f %9.6f %5d\n", ra, dec, sp->p);
	    else if (hflag) {
		double mag, dmag;
		double apra, apdec;
		double alt, az;
		double ha;

		apra = ra;
		apdec = dec;
		as_ap (np, eq, &apra, &apdec);
		ha = lst - apra;
		hadec_aa (lat, ha, apdec, &alt, &az);
		refract (pressure, temp, alt, &alt);

		starMag (&ssp[0], sp, &mag, &dmag);
printf ("%d %g %d %g\n", ssp[0].Src, ssp[0].rmsSrc, sp->Src, sp->rmsSrc);

		printf ("%9.6f %9.6f  %9.6f %9.6f  %5.2f  %5.2f %5.2f  %14.6f %6.1f\n",
			    raddeg(alt), raddeg(az), raddeg(ra), raddeg(dec),
			    mag, sp->xfwhm, sp->yfwhm, mjd+MJD0-expt/SPD, expt);
	    } else {
		char rastr[32], decstr[32];
		// 021001: Added extra digit to both RA and DEC
		fs_sexa (rastr, radhr (ra), 2, 360000);
		fs_sexa (decstr, raddeg (dec), 3, 36000);
		printf ("%s %s %5d\n", rastr, decstr, ssp[i].p);
	    }
	}
	if (ns >= 0)
	    free ((void *)ssp);

	resetFImage (fip);
}

#if USE_DISTANCE_METHOD


/* get astrometric fit parameters AMDX*, AMDY* from FITS header
 * return 0 if all found (all must be present, even if zero), else -1
 */
static int
getAMDFITS (FImage *fip, double a[], double b[])
{
  char name[7];
  int i;

  for (i = 0; i < 14; i++) {
    sprintf (name, "AMDX%i", i);
    if (getRealFITS (fip, name, &a[i]) < 0) return (-1);
    sprintf (name, "AMDY%i", i);
    if (getRealFITS (fip, name, &b[i]) < 0) return (-1);
  }

  return (0);
}


/* Pixel coordinates to "standard coordinates" in radians,
 * assuming a1-13, b1-13 set correctly
 */
static void
xy2xieta(double a[], double b[], double x, double y, double *xi, double *eta)
{
  double x2y2 = x*x + y*y;
  *xi  = a[1]*x + a[2]*y + a[3] + a[4]*x*x + a[5]*x*y + a[6]*y*y +
         a[8]*x*x*x + a[9]*x*x*y + a[10]*x*y*y + a[11]*y*y*y +
         a[7]*x2y2 + a[12]*x*x2y2 + a[13]*x*x2y2*x2y2;
  *eta = b[1]*y + b[2]*x + b[3] + b[4]*y*y + b[5]*x*y + b[6]*x*x +
         b[8]*y*y*y + b[9]*x*y*y + b[10]*x*x*y + b[11]*x*x*x +
         b[7]*x2y2 + b[12]*y*x2y2 + b[13]*y*x2y2*x2y2;
}


/* RA, dec of projection centre + n (xi,eta)'s to n (RA,dec)'s (all radians)
 * Input (xi,eta)'s given as pointers so that this function can be called for
 * arrays (setting n=1) or single stars
 */
static void
xieta2RADec (double rc, double dc, int n, double *xi, double *eta,
             double *r, double *d)
{
  int i;
  double tdc, *rp, *dp, *xip, *etap;
  tdc = tan(dc);
  for (i = 0, rp = r, dp = d, xip = xi, etap = eta;  i < n;
       i++,   rp++,   dp++,   xip++,    etap++) {
    *rp = atan( (*xip/cos(dc))/(1-*etap*tdc) ) + rc;
    *dp = atan( (*etap+tdc)*cos(*r-rc)/(1-*etap*tdc) );
  }
}
#endif
