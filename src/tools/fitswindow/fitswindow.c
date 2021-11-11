/* read a 16 bit FITS image from stdin and produce a 16 but FITS image on stdout
 * which only has pixel values from 0..255. These are mapped from the original
 * image from mean - sd .. mean + 2*sd. The stats ignore a small border.
 * the image header is passed through unchanged except we remove any BZERO and
 * BSCALE keywords and add one which indicates the lo and hi values.
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "fits.h"

#define	BORD	32	/* size of border to ignore when doing stats */

static void doWindow (int in, int out);
static void mkLUT (CamPixel lut[NCAMPIX], int lo, int hi);

int
main(int ac, char *av[])
{
	/* check usage */
	if (ac != 1) {
	    fprintf (stderr, "Usage: %s < in.fts > out.fts\n", av[0]);
	    exit (1);
	}

	/* do stdin to stdout */
	doWindow (0, 1);

	return (0);
}

static void
doWindow (int in, int out)
{
	char msg[1024];
	AOIStats s;
	FImage f;
	CamPixel lut[NCAMPIX];
	CamPixel *ip, *lip;
	int lo, hi;

	/* read file */
	if (readFITS (in, &f, msg) < 0) {
	    fprintf (stderr, "Error reading: %s\n", msg);
	    exit(1);
	}

	/* find stats */
	aoiStatsFITS (f.image, f.sw, BORD, BORD, f.sw-2*BORD, f.sh-2*BORD, &s);
	lo = (int)s.mean - s.sd;
	hi = (int)s.mean + 2*s.sd;

	/* create a lut */
	mkLUT (lut, lo, hi);

	/* pass each pixel through the lut */
	ip = (CamPixel *)f.image;
	lip = ip + f.sw*f.sh;
	for (; ip < lip; ip++)
	    *ip = lut[*ip];

	/* discard a few fields */
	(void) delFImageVar (&f, "BZERO");
	(void) delFImageVar (&f, "BSCALE");

	/* add a comment about the range */
	(void) sprintf (msg, "Original Contrast range: %d .. %d (of 65535)\n",
									lo, hi);
	setCommentFITS (&f, "COMMENT", msg);

	/* write out new version */
	if (writeFITS (out, &f, msg, 0) < 0) {
	    fprintf (stderr, "Error writing: %s\n", msg);
	    exit(1);
	}
}

static void
mkLUT (CamPixel lut[NCAMPIX], int lo, int hi)
{
	int i, n;

	for (i = 0; i < lo; i++)
	    lut[i] = 0;

	n = hi - lo;
	for ( ; i < hi; i++)
	    lut[i] = (i - lo)*256/n;

	for ( ; i < NCAMPIX; i++)
	    lut[i] = lut[hi-1];
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: fitswindow.c,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $"};
