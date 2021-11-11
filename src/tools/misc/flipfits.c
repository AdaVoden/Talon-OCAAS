/* utility to flip rows and/or columns of a FITS image.
 * this just does the pixels -- not the header fields.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "fits.h"

static void usage(char *prognam);
static int doFlipping (char *fn);
static int openImage (char *name, FImage *fip, char msg[]);
static int writeImage (char *name, FImage *fip, char msg[]);

static int rflag;
static int cflag;
static int xflag,xdir;

int
main (int ac, char *av[])
{
	char *progname = av[0];
	char *str;
	char d;
	int nbad;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str))
		switch (c) {
		case 'r':	/* flip rows */
		    rflag++;
		    break;
		case 'c':	/* flip columns */
		    cflag++;
		    break;
		case 'x':	/* transpose rows and columns */
			d = *++str;
			switch(d) {
				case 'l':
					xdir++;
					break;
				case 'r':
					xdir--;
					break;
			}
			xflag++;
			break;		
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining file names starting at av[0] */

	if (ac == 0) {
	    fprintf (stderr, "No files\n");
	    usage (progname);
	}

	if (!rflag && !cflag && !xflag) {
	    fprintf (stderr, "Set at least one of -r, -c or -x\n");
	    usage (progname);
	}

	nbad = 0;
	while (ac-- > 0) {
	    if (doFlipping (*av++) < 0)
		nbad++;
	}

	return (nbad);
}

static void
usage(char *progname)
{
	FILE *fp = stderr;

	fprintf(fp,"%s: usage: [options] file.fts ...\n", progname);
	fprintf(fp,"  -r: flip rows.\n");
	fprintf(fp,"  -c: flip columns.\n");
	fprintf(fp,"  -xl: transpose rows & columns, rotating left.\n");
	fprintf(fp,"  -xr: transpose rows & columns, rotating right.\n");
	fprintf(fp,"\n");
	fprintf(fp,"N.B. this program does NOT edit the FITS headers\n");
	fprintf(fp,"(except to swap axis sizes on transpositions)\n");
	fprintf(fp,"But it does add HISTORY entries denoting the flipping.\n");
	exit (1);
}

/* open fn, flip according to rflag and cflag, and close.
 * return 0 if ok, else -1.
 */
static int
doFlipping (char *fn)
{
	FImage fim, *fip = &fim;
	char msg[1024];
	int w, h;

	if (openImage (fn, fip, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, msg);
	    return (-1);
	}
	if (getNAXIS (fip, &w, &h, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, msg);
	    resetFImage (fip);
	    return (-1);
	}

	if (rflag) {
	    flipImgRows ((CamPixel *)fip->image, w, h);
	    setCommentFITS (fip, "HISTORY", "Rows have been flipped");
	}
	if (cflag) {
	    flipImgCols ((CamPixel *)fip->image, w, h);
	    setCommentFITS (fip, "HISTORY", "Columns have been flipped");
	}
	if(xflag) {
		transposeXY((CamPixel *)fip->image, w, h, xdir);
		sprintf(msg,"Image rotated 90 degrees %s",xdir < 0 ? "clockwise" : "counter-clockwise");
		setCommentFITS( fip, "HISTORY", msg);		
	
		/* reverse these in the header, since we flipped 'em */
		setIntFITS(fip,"NAXIS1",h,"Number of columns");
		setIntFITS(fip,"NAXIS2",w,"Number of rows");	
	}

	if (writeImage (fn, fip, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, msg);
	    resetFImage (fip);
	    return (-1);
	}

	resetFImage (fip);
	return (0);
}

/* open the given FITS file for read/write.
 * open r/w now just to make sure we can later.
 * if ok return file des, else return -1 with excuse in msg[].
 */
static int
openImage (char *name, FImage *fip, char msg[])
{
	int fd;

	fd = open (name, O_RDWR);
	if (fd < 0) {
	    strcpy (msg, strerror (errno));
	    return (-1);
	}

	if (readFITS (fd, fip, msg) < 0) {
	    (void) close (fd);
	    return (-1);
	}

	(void) close (fd);
	return (fd);
}

/* write the given fits file.
 * if ok return 0, else return -1 with excuse in msg[].
 */
static int
writeImage (char *fn, FImage *fip, char msg[])
{
	int fd;

	fd = open (fn, O_RDWR|O_TRUNC);
	if (fd < 0) {
	    strcpy (msg, strerror (errno));
	    return (-1);
	}

	if (writeFITS (fd, fip, msg, 0) < 0) {
	    (void) close (fd);
	    return (-1);
	}

	(void) close (fd);
	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: flipfits.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
