/* scan a directory supposedly containing standard photometric field images
 * and dig out all the star stats.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "wcs.h"
#include "photstd.h"

#include "photcal.h"

#define	RMAX		15	/* max radius from brghtst pixel to star edge */
#define	MAXERR		0.3	/* max error allowed in a star */

static int fitsFilename (char path[]);
static void processFile (char fullfn[], char fn[], double jd, double djd,
    PStdStar *sp, int nsp);
static void processStar (char fn[], FImage *fip, double Z, int filter,
    double dur, PStdStar *stp);
static void addOneStar (char fn[], OneStar **opp, int *np, double M,
    double err, double Z);

/* scan the given directory for all images with JD +/- djd days of the
 *   given jd whose OBJECT field matches one of sfp->o.o_name. Then find the
 *   airmass and dig out the stars in the image and determine their observed
 *   magnitudes.
 * as strange things are found report but keep going as much as psbl.
 * return 0 if anything at all seems to work. return -1 only if nothing works.
 */
int
scanDir (char dir[], double jd, double djd, PStdStar *sp, int nsp)
{
	DIR *dirp;
	struct dirent *dp;

	dirp = opendir (dir);
	if (!dirp) {
	    fprintf (stderr, "%s: %s\n", dir, strerror(errno));
	    return (-1);
	}

	if (verbose)
	    printf ("Scanning %s for images.\n", dir);
	while ((dp = readdir (dirp)) != NULL) {
	    if (fitsFilename (dp->d_name) == 0) {
		char full[2048];
		sprintf (full, "%s/%s", dir, dp->d_name);
		processFile (full, dp->d_name, jd, djd, sp, nsp);
	    }
	}

	(void) closedir (dirp);
	return (0);
}

/* return 0 if the given filename ends with .fts, else -1. */
static int
fitsFilename (char path[])
{
	int l = strlen (path);
	char *suf = path+l-4;

	return (l>4 && (!strcmp (suf, ".fts") || !strcmp(suf,".FTS")) ? 0 : -1);
}

static void
processFile (char fullfn[], char fn[], double jd, double djd, PStdStar *sp,
int nsp)
{
	char msg[1024];
	FImage fim, *fip = &fim;
	FITSRow row;
	PStdStar *fsp;
	double el, Z;
	double ijd;
	double dur;
	int filter;
	int fd;
	int i;

	if (verbose)
	    printf ("Reading %s\n", fullfn);

	/* open the FITS file */
	fd = open (fullfn, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", fullfn, strerror(errno));
	    return;
	}
	if (readFITSHeader (fd, fip, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fullfn, msg);
	    close (fd);
	    return;
	}

	/* confirm image has all WCS fields */
	if (checkWCSFITS (fip, 0) < 0) {
	    printf ("%s: no WCS fields\n", fullfn);
	    goto done;
	}

	/* get the object name and set fsp if it is in the PStdStar list */
	if (getStringFITS (fip, "OBJECT", row) < 0) {
	    printf ("%s: No OBJECT field\n", fullfn);
	    goto done;
	}
	for (i = 0; i < nsp; i++)
	    if (strcasecmp (sp[i].o.o_name, row) == 0)
		break;
	if (i == nsp) {
	    printf ("%s: OBJECT not in Landolt list: %s\n", fullfn, row);
	    goto done;
	}
	fsp = sp + i;

	/* see if date is in range */
	if (getRealFITS (fip, "JD", &ijd) < 0) {
	    printf ("%s: no JD field\n", fullfn); 
	    goto done;
	}
	if (fabs(jd - ijd) > djd)
	    goto done;

	/* compute airmass */
	if (getStringFITS (fip, "ELEVATIO", row) < 0) {
	    printf ("%s: no ELEVATIO field\n", fullfn); 
	    goto done;
	}
	if (scansex (row, &el) < 0) {
	    printf ("%s: bad ELEVATIO field: %s\n", fullfn, row); 
	    goto done;
	}
	Z = 1.0/sin(degrad(el));

	/* get filter */
	if (getStringFITS (fip, "FILTER", row) < 0) {
	    printf ("%s: no FILTER field.\n", fullfn); 
	    goto done;
	}
	filter = (int)row[0];

	/* get exposure time */
	if (getRealFITS (fip, "EXPTIME", &dur) < 0) {
	    printf ("%s: no EXPTIME field.\n", fullfn); 
	    goto done;
	}

	/* Phew! now it's worth reading the pixels and dig out the star stats */
	resetFImage (fip);
	lseek (fd, 0, 0);
	if (readFITS (fd, fip, msg) < 0) {
	    printf ("%s: %s\n", fullfn, msg);
	    goto done;
	}
	if (verbose > 1)
	    printf ("%s: Filter=%c Z=%g\n", fullfn, filter, Z);
	processStar (fn, fip, Z, filter, dur, fsp);

	/* ok */
    done:
	resetFImage (fip);
	close (fd);
}

static void
processStar (char fn[], FImage *fip, double Z, int filter, double dur,
PStdStar *stp)
{
	char msg[1024];
	StarDfn sd;
	StarStats ss;
	double x, y;
	double M, err;

	/* find image coords */
	if (RADec2xy (fip, stp->o.f_RA, stp->o.f_dec, &x, &y) < 0) {
	    printf("%s: can not convert RA/Dec (%g/%g, rads) to X/Y\n",
						fn, stp->o.f_RA, stp->o.f_dec);
	    return;
	}

	sd.rsrch = RMAX;
	sd.rAp = ABSAP;
	sd.how = SSHOW_MAXINAREA;

	if (starStats ((CamPixel *)fip->image, fip->sw, fip->sh, &sd, (int)x,
						    (int)y, &ss, msg) < 0) {
	    printf ("%s: trouble finding star stats: %s\n", fn, msg);
	    return;
	}

	if (verbose > 1)
	    printf ("%s: Start x/y: [%4.0f,%4.0f] -> Brightest at [%4.0f,%4.0f]\n",
							fn, x, y, ss.x, ss.y);

	/* find the observed magnitude, normalized to exposure time.
	 * this is the same as use in camera's absolute photometry code.
	 * also an error estimate.
	 */
	M = -2.511886 * log10(ss.Src/dur);

	/* pre-aperture photometry
	err = 2.2 / (((double)ss.p - (double)ss.nmed)/(double)ss.nsd);
	 */

	err = 2.2 / (((double)ss.p - (double)ss.Sky)/(double)ss.rmsSky);
	if (err > MAXERR) {
	    printf ("%s: %s %c error too large: %g\n", fn, stp->o.o_name,
								filter, err);
	    return;
	}

	if (verbose > 1)
	    printf ("%s: %c: M=%g Z=%g err=%g x=%3d y=%3d max=%5d rAp=%1d Src=%6d rmsSrc=%5.1f Sky=%4d rmsSky=%5.1f\n",
		fn, filter, M, Z, err, ss.bx, ss.by, ss.p, ss.rAp,
		ss.Src, ss.rmsSrc, ss.Sky, ss.rmsSky);

	/* store in the proper place, depending on filter */
	switch (filter) {
	case 'B': addOneStar (fn, &stp->Bp, &stp->nB, M, err, Z); break;
	case 'V': addOneStar (fn, &stp->Vp, &stp->nV, M, err, Z); break;
	case 'R': addOneStar (fn, &stp->Rp, &stp->nR, M, err, Z); break;
	case 'I': addOneStar (fn, &stp->Ip, &stp->nI, M, err, Z); break;
	default: printf ("%s: unknown filter: %c\n", fn, filter);
	}
}

/* grow the OneStar list by one and set Vobs to M and and Z to Z.
 */
static void
addOneStar (char fn[], OneStar **opp, int *np, double M, double err, double Z)
{
	int n = *np;
	OneStar *sp;

	if (*opp)
	    sp = (OneStar *) realloc((char *)(*opp), (n+1) * sizeof(OneStar));
	else
	    sp = (OneStar *) malloc (sizeof(OneStar));
	if (!sp) {
	    fprintf (stderr, "No more memory\n");
	    exit (1);
	}
	*opp = sp;
	sp += n;
	memset ((char *)sp, 0, sizeof(OneStar));

	sp->Vobs = M;
	sp->Z = Z;
	sp->Verr = err;
	strncpy (sp->fn, fn, sizeof(sp->fn)-1);

	(*np) = n + 1;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: scandir.c,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $"};
