/* utility to create a fake FITS file on stdout.
 * we only support INTEGER*16 data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"

static void usage(char *p);

int
main (int ac, char *av[])
{
	char *program = av[0];
	int r, c;
	int nbytes;
	int v;
	unsigned short *pix;
	unsigned short usv;
	int i;

	if (ac != 4)
	    usage (program);

	r = atoi(av[1]);
	c = atoi(av[2]);
	v = atoi(av[3]);
	if (v < 0 || v >= 65536) {
	    fprintf (stderr, "Value must be 0..65535\n");
	    exit (1);
	}

	nbytes = r*c * sizeof(unsigned short);
	pix = (unsigned short *) malloc (nbytes);
	if (!pix) {
	    fprintf (stderr, "Can not malloc %d bytes\n", nbytes);
	    exit (1);
	}

	usv = v;
	for (i = r*c; --i >= 0; )
	    pix[i] = usv;

	if (writeSimpleFITS (1, (char *)pix, c, r, 0, 0, 0, 0) < 0) {
	    fprintf (stderr, "Error creating new FITS file\n");
	    exit (1);
	}

	return (0);
}

static void
usage(p)
char *p;
{
	FILE *f = stderr;

	fprintf (f, "%s: rows cols value\n", p);
	fprintf (f, "  create new image on stdout of given size with all pixels set to value\n");
	fprintf (f, "  we set SIMPLE BITPIX NAXIS NAXIS1 NAXIS2 and BZERO\n");

	exit (1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: mkfits.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
