#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "photom.h"

static int computeStarStats (FImage *fip, int n);
static int oneStarStat (FImage *fip, int f, int i);

/* go through the list of files and generate the output for each.
 * stars in each image are compared to the last star in that image.
 * the last star in each image is compared to the last star in the first image.
 * we assume all the files[i].sl[j].{x,y} are all set up.
 */
void
doAllImages()
{
	FImage fimage, *fip = &fimage;
	char *fn;
	int i, f, s;
	double pixdc0;

	/* print the column headings once */
	printHeader();

	/* process each image.
	 * N.B. the stats for the reference star are done first to establish
	 *   an aperture. We exit if star stats for this star can not be
	 *   determined.
	 */
	initFImage (fip);
	for (i = f = 0; f < nfiles; f++) {
	    fn = files[f].fn;
	    if (readFITSfile (fn, 0, fip) < 0)
		continue;
	    if (getRealFITS (fip, "PIXDC0", &pixdc0) < 0)
		pixdc0 = 0.0;
	    files[f].pixdc0 = pixdc0;
	    if (f == 0) {
		if (oneStarStat (fip, f, nstars-1) < 0) {
		    printf ("Can not compute stats even for reference star -- exiting\n");
		    exit(1);
		}
		stardfn.rAp = files[0].ss[nstars-1].rAp;
		if (verbose)
		    printf ("Aperture = %d pixels\n", stardfn.rAp);
	    }
	    if (computeStarStats (fip, f) < 0)
		continue;
	    printImageHeader (i, fip);	/* must follow computeStarStats() */
	    for (s = 0; s < nstars-1; s++)
		printStarReport (&files[f].ss[nstars-1], &files[f].ss[s]);
	    printStarReport (&files[0].ss[nstars-1], &files[f].ss[nstars-1]);
	    printf ("\n");
	    resetFImage (fip);
	    i++;
	}
}

/* compute stats of stars in fip near files[n].sl into files[n].ss.
 * return 0 if any ok, else -1.
 */
static int
computeStarStats (fip, f)
FImage *fip;
int f;
{
	int ok = 0;
	int i;

	for (i = 0; i < nstars; i++)
	    if (oneStarStat (fip, f, i) == 0)
		ok = 1;

	return (ok ? 0 : -1);
}

/* compute the stats for the given star on the given file.
 * we also require fwhm to be in range 1..MFWHM.
 * return 0 if ok, else -1
 */
static int
oneStarStat (fip, f, i)
FImage *fip;
int f;	/* file index */
int i;	/* star index */
{
	StarStats *ssp = &files[f].ss[i];
	char errmsg[1024];
	CamPixel *p;
	int w, h;

	w = fip->sw;
	h = fip->sh;
	p = (CamPixel *) fip->image;

	if (starStats(p, w, h, &stardfn, files[f].sl[i].x, files[f].sl[i].y,
							ssp, errmsg) < 0) {
	    if (verbose)
		printf ("%s: Bad starStats for star %d: %s\n", files[f].fn,
								i, errmsg);
	    memset ((char *)ssp, 0, sizeof(StarStats));
	    return (-1);
	} else if (sqrt(ssp->xfwhm * ssp->yfwhm) > MFWHM) {
	    if (verbose)
		printf("%s: FWHM too large to be believed for star %d: %g %g\n",
				    files[f].fn, i, ssp->xfwhm, ssp->yfwhm);

	    memset ((char *)ssp, 0, sizeof(StarStats));
	    return (-1);
	} else if (ssp->xfwhm <= 1 || ssp->yfwhm <= 1) {
	    if (verbose)
		printf ("%s: FWHM < 1 for star %d\n", files[f].fn, i);
	    memset ((char *)ssp, 0, sizeof(StarStats));
	    return (-1);
	} else {
	    if (verbose) {
		printf ("%2d:", i);
		printStarStats (&files[f].ss[i]);
	    }
	}

	/* ok */
	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: doimages.c,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $"};
