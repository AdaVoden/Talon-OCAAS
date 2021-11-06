/* handle the cropping option.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "wcs.h"
#include "photom.h"

#define	CROPBDR	30	/* extra pixels to keep outside bounding box */

static void cropit (char *fn, int x, int y, int w, int h);

/* go through the list of all the StarStats and find a net bounding box.
 * then crop each of the input files IN PLACE to this box plus a border.
 */
void
doCropping()
{
	int minx, miny, maxx, maxy;
	int totdx, totdy;
	int f, s;
	int x, y;

	/* find overall bounding box.
	 */
	minx = maxx = (int)files[0].ss[0].x;
	miny = maxy = (int)files[0].ss[0].y;
	totdx = totdy = 0;
	for (f = 0; f < nfiles; f++) {
	    totdx += files[f].dx;
	    totdy += files[f].dy;
	    for (s = 0; s < nstars; s++) {
		x = (int)files[f].ss[s].x + totdx;
		if (x > maxx) maxx = x;
		if (x < minx) minx = x;
		y = (int)files[f].ss[s].y + totdy;
		if (y > maxy) maxy = y;
		if (y < miny) miny = y;
	    }
	}

	/* crop each image to that box */
	totdx = totdy = 0;
	for (f = 0; f < nfiles; f++) {
	    totdx += files[f].dx;
	    totdy += files[f].dy;
	    cropit (files[f].fn, minx - totdx, miny - totdy,
						    maxx-minx+1, maxy-miny+1);
	}
}

static void
cropit (fn, x, y, w, h)
char *fn;
int x, y, w, h;
{
	FImage fip0, fip1;
	int sw, sh;
	char errmsg[1024];
	int fd;

	initFImage (&fip0);
	initFImage (&fip1);

	/* read the file; ignore errors since it worked a minute ago :-) */
	fd = open (fn, O_RDONLY);
	(void) readFITS (fd, &fip0, errmsg);
	close (fd);

	/* don't crop again */
	if (getIntFITS (&fip0, "CROPX", &sw) == 0) {
	    resetFImage (&fip0);
	    printf ("%s: Already cropped\n", fn);
	    return;
	}

	/* get size */
	if (getNAXIS (&fip0, &sw, &sh, errmsg) < 0) {
	    resetFImage (&fip0);
	    fprintf (stderr, "Can not get size of %s: %s\n", fn, errmsg);
	    return;
	}

	/* check bounds */
	if (x < 0) x = 0;
	if (x >= sw) x = sw-1;
	if (x + w >= sw) w = sw-x;
	if (y < 0) y = 0;
	if (y >= sh) y = sh-1;
	if (y + h >= sh) h = sh-y;

	/* tweak bounds for a border */
	if (x >= CROPBDR) {x -= CROPBDR; w += CROPBDR;}
	if (x + w + CROPBDR < sw) {w += CROPBDR;}
	if (y >= CROPBDR) {y -= CROPBDR; h += CROPBDR;}
	if (y + h + CROPBDR < sh) {h += CROPBDR;}

	/* crop it */
	if (cropFITS (&fip1, &fip0, x, y, w, h, errmsg) < 0) {
	    resetFImage (&fip0);
	    resetFImage (&fip1);
	    fprintf (stderr, "Error cropping %s: %s\n", fn, errmsg);
	    return;
	}

	/* open again for writing, with truncate */
	fd = open (fn, O_RDWR|O_TRUNC);
	if (fd < 0) {
	    resetFImage (&fip0);
	    resetFImage (&fip1);
	    fprintf (stderr, "Can not open %s for writing\n", fn);
	    return;
	}

	/* write new cropped image */
	if (writeFITS(fd, &fip1, errmsg, 0) < 0) {
	    resetFImage (&fip0);
	    resetFImage (&fip1);
	    close (fd);
	    fprintf (stderr, "Error writing cropped version of %s: %s\n", fn,
								errmsg);
	    return;
	}

	close (fd);
	resetFImage (&fip0);
	resetFImage (&fip1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: cropping.c,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $"};
