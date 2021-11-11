/* Crop is a program which crops a set of FITS images to the largest common AOI.
 * Or, crop can be given a file which lists OBJECT names and AOIs and it will
 * crop each image to that AOI. Or, crop can be given an AOI on its command line
 * and it will crop all images to that. Or, crop can be given any number of
 * rectangles to extract in pixel coordinates.
 *
 * N.B. Except for the rectangle extractor, crop creates the new cropped
 * images IN PLACE. Copy the images first if you want them preserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>	/* we use Regions */
#include <X11/Xutil.h>	/* we use Regions */

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "strops.h"
#include "fits.h"
#include "wcs.h"

#undef MAX
#define	MAX(a,b)	((a)>(b)?(a):(b))
#undef MIN
#define	MIN(a,b)	((a)<(b)?(a):(b))

typedef struct {
    double er, wr;		/* east and west max ra, rads */
    double nd, sd;		/* north and south max dec, rads */
} CropAOI;

typedef struct {
    int x, y, w, h;		/* pixel coords */
} FITSRect;
static FITSRect *frects;	/* malloced array of nfrects */
static int nfrects;

static void usage (char *p);
static int crackAOI (char *str, CropAOI *aoip);
static void doAoption (int ac, char *av[], CropAOI *aoip);
static void doCoption (int ac, char *av[]);
static void doFoption (int ac, char *av[], char *aoifile);
static void doPoption (int ac, char *av[]);
static void addFRect (int x, int y, int w, int h);
static void cropOne (char *fn, CropAOI *aoip);
static void printRegion (char *msg, Region reg);
static Region fitsRegion (FImage *fip);
static Region fitsSubRegion (FImage *fip0, FImage *fipi);
static void region2CropAOI (FImage *fip, Region r, CropAOI *ap);

int verbose;

int
main (int ac, char *av[])
{
	char *progname = av[0];
	int aflag = 0, cflag = 0, fflag = 0;
	int nfiles = 0;
	char **fnames;
	CropAOI aoi;
	char *aoifile = 0;
	char *str;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    /* in the loop ac includes the current arg */
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'a':	/* explicit AOI */
		    if (ac < 2)
			usage(progname);
		    if (crackAOI (*++av, &aoi) < 0) {
			fprintf (stderr, "Bad AOI format\n");
			usage (progname);
		    }
		    ac--;
		    aflag++;
		    break;
		case 'c':	/* crop to their own relative max AOI */
		    cflag++;
		    break;
		case 'l':	/* filename listing OBJECT and AOIs */
		    if (ac < 2)
			usage(progname);
		    aoifile = *++av;
		    fflag++;
		    ac--;
		    break;
		case 'f':	/* list of files to crop is in a file */
		    if (ac < 2)
			usage(progname);
		    nfiles = readFilenames (av[1], &fnames);
		    ac--;
		    av++;
		    break;
		case 'p':	/* rectange to extract */
		    if (ac < 5)
			usage(progname);
		    addFRect(atoi(av[1]), atoi(av[2]), atoi(av[3]),atoi(av[4]));
		    ac -= 4;
		    av += 4;
		    break;
		case 'v':	/* verbose */
		    verbose++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0] */

	if (nfiles > 0 && ac > 0) {
	    fprintf (stderr, "Can not give files and use -f too.\n");
	    usage (progname);
	}
	if (!!frects + aflag + cflag + fflag != 1) {
	    fprintf (stderr, "Must specify exactly one of -a -c -l or -p\n");
	    usage (progname);
	}

	/* use ac/av even if read filenames from a file */
	if (nfiles) {
	    ac = nfiles;
	    av = fnames;
	}

	if (aflag)
	    doAoption(ac, av, &aoi);
	if (cflag)
	    doCoption(ac, av);
	if (fflag)
	    doFoption(ac, av, aoifile);
	if (frects)
	    doPoption(ac, av);

	return (0);
}

void
usage (char *p)
{
    FILE *fp = stderr;

    fprintf (fp, "Usage: %s [options] [*.fts]\n", p);
    fprintf (fp, "Purpose: crop images to a standard, common or given AOI.\n");
    fprintf (fp, "N.B: all files are modified IN PLACE.\n");
    fprintf (fp, "Options:\n");
    fprintf (fp, "  -a AOI:  crops all images to the given AOI. The AOI is in the form:\n");
    fprintf (fp, "           \"East_RA North_Dec West_RA South_Dec\", RA as H:M:S, Dec as D:M:S.\n");
    fprintf (fp, "  -c:      crops all images to their maximum common AOI.\n");
    fprintf (fp, "  -l file: crops to AOI for matching OBJECTs listed in given file.\n");
    fprintf (fp, "           file format is: OBJECT East_RA North_Dec West_RA South_Dec.\n");
    fprintf (fp, "  -f file: list of files to crop.\n");
    fprintf (fp, "  -p x y w h:  extract and create new file; use as many as desired\n");
    fprintf (fp, "           each filename will be original + hex count\n");
    fprintf (fp, "  -v:      verbose\n");
    fprintf (fp, "\n");

    exit (1);
}

/* break an AOI string into its four components.
 * return 0 if ok else -1.
 */
int
crackAOI (char *str, CropAOI *aoip)
{
	char erstr[64], ndstr[64], wrstr[64], sdstr[64];
	double er, nd, wr, sd;

	while (*str == ' ') str++;
	if (sscanf (str, "%s %s %s %s", erstr, ndstr, wrstr, sdstr) != 4)
	    return (-1);

	if (scansex (erstr, &er) < 0)
	    return (-1);
	aoip->er = hrrad (er);
	if (scansex (ndstr, &nd) < 0)
	    return (-1);
	aoip->nd = degrad (nd);
	if (scansex (wrstr, &wr) < 0)
	    return (-1);
	aoip->wr = hrrad (wr);
	if (scansex (sdstr, &sd) < 0)
	    return (-1);
	aoip->sd = degrad (sd);

	return (0);
}

/* crop all images to the given AOI */
void
doAoption (int ac, char *av[], CropAOI *aoip)
{
	while (ac--)
	    cropOne (*av++, aoip);
}

/* crop all images to their maximum common AOI */
void
doCoption (int ac, char *av[])
{
	FImage fim0, *fip0 = &fim0;
	FImage fimi, *fipi = &fimi;
	Region reg0 = NULL;
	CropAOI aoi;
	int i;

	/* init reg0 to full size of first image.
	 * keep reducing by intersecting with regions of each subsequent
	 *   image, always computed in pixel coords of first image.
	 */
	initFImage (fip0);
	initFImage (fipi);
	for (i = 0; i < ac; i++) {
	    char *fn = av[i];	/* file name */
	    char msg[1024];	/* message buffer */
	    int fd;		/* file descriptor of open file */

	    /* open the FITS file and read the header */
	    fd = open (fn, O_RDONLY);
	    if (fd < 0) {
		fprintf (stderr, "%s: %s\n", fn, strerror(errno));
		exit (1);
	    }
	    resetFImage (fipi);
	    if (readFITSHeader (fd, fipi, msg) < 0) {
		fprintf (stderr, "%s: %s\n", fn, msg);
		exit (1);
	    }
	    close (fd);

	    /* must have WCS headers */
	    if (checkWCSFITS (fipi, 0) < 0) {
		fprintf (stderr, "%s: no WCS headers\n", fn);
		exit (1);
	    }

	    /* update, or create if first, clip region */
	    if (!reg0) {
		/* init region to whole of fipi, and save in fip0 */
		reg0 = fitsRegion (fipi);
		copyFITS (fip0, fipi);
		if (verbose)
		    printRegion (fn, reg0);
	    } else {
		/* intersect fipi region with reg0 */
		Region regi = fitsSubRegion (fip0, fipi);
		Region newreg = XCreateRegion();
		if (verbose)
		    printRegion (fn, regi);
		XIntersectRegion (reg0, regi, newreg);
		XDestroyRegion (reg0);
		XDestroyRegion (regi);
		reg0 = newreg;
	    }
	    if (verbose)
		printRegion ("Clip box now", reg0);

	    resetFImage (fipi);
	}

	/* insure we found something */
	if (!reg0 || XEmptyRegion(reg0)) {
	    fprintf (stderr, "Images have no area in common.\n");
	    exit (1);
	}
	if (verbose)
	    printRegion ("Cropping to", reg0);

	/* create a CropAOI from net region */
	region2CropAOI (fip0, reg0, &aoi);

	/* now crop everything to reg0 */
	for (i = 0; i < ac; i++)
	    cropOne (av[i], &aoi);

	XDestroyRegion (reg0);
	resetFImage (fip0);
}

/* crop images to the AOI with the matching OBJECT in aoifile */
void
doFoption (int ac, char *av[], char *aoifile)
{
	FILE *aoifp;
	FImage fim;

	/* open the aoi file */
	aoifp = fopen (aoifile, "r");
	if (!aoifp) {
	    fprintf (stderr, "%s: can not open\n", aoifile);
	    exit (1);
	}

	/* for each image
	 *   open it
	 *   get OBJECT
	 *   get AOI from aoifile with same OBJECT
	 *   crop
	 */
	initFImage (&fim);
	while (ac--) {
	    char *fn = *av++;
	    char msg[1024];
	    char objname[128];
	    int found;
	    CropAOI aoi;
	    int fd;

	    if (verbose)
		printf ("%s: opening\n", fn);

	    /* open the FITS file */
	    fd = open (fn, O_RDONLY);
	    if (fd < 0) {
		fprintf (stderr, "%s: Can not open\n", fn);
		continue;
	    }

	    /* read the header */
	    resetFImage (&fim);
	    if (readFITSHeader (fd, &fim, msg) < 0) {
		fprintf (stderr, "%s: %s\n", fn, msg);
		resetFImage (&fim);
		close (fd);
		continue;
	    }
	    close (fd);

	    /* dig out OBJECT */
	    if (getStringFITS (&fim, "OBJECT", objname) < 0) {
		fprintf (stderr, "%s: no OBJECT field\n", fn);
		resetFImage (&fim);
		continue;
	    }

	    /* finished with fim */
	    resetFImage (&fim);

	    /* scan aoifile for objname */
	    found = 0;
	    rewind (aoifp);
	    while (!found && fgets (msg, sizeof(msg), aoifp)) {
		char name[64];

		if (sscanf (msg, "%s", name) != 1)
		    continue;
		if (strcwcmp (objname, name) != 0)
		    continue;

		if (crackAOI (msg+strlen(name), &aoi) < 0) {
		    fprintf (stderr, "%s: Bad AOI entry for %s\n",aoifile,name);
		    break;
		}

		found = 1;
	    }
	    if (!found) {
		fprintf (stderr, "%s: no AOI found in %s\n", objname, fn);
		continue;
	    }

	    /* overwrite with cropped version */
	    cropOne (fn, &aoi);

	    if (verbose)
		printf ("%s: cropped and close.\n", fn);
	}

	fclose (aoifp);
}

/* extract and copy each frect from each file to a new file */
static void
doPoption (int ac, char *av[])
{
	FImage fim, cfim;

	/* loop through files to be cropped */
	initFImage (&fim);
	initFImage (&cfim);
	while (ac--) {
	    char *fn = *av++;
	    char buf[1024];
	    char *pt;
	    int i, fd;

	    /* open the master FITS file */
	    if (verbose)
		printf ("%s: opening\n", fn);
	    fd = open (fn, O_RDONLY);
	    if (fd < 0) {
		fprintf (stderr, "%s: %s\n", fn, strerror(errno));
		continue;
	    }

	    /* read */
	    if (readFITS (fd, &fim, buf) < 0) {
		fprintf (stderr, "%s: %s\n", fn, buf);
		close (fd);
		continue;
	    }

	    /* prep for creating new name */
	    pt = strrchr (fn, '.');

	    /* create each cropped portion */
	    for (i = 0; i < nfrects; i++) {
		FITSRect *rp = &frects[i];
		char cfn[1024];
		int cfd;

		/* create file */
		if (pt)
		    sprintf (cfn, "%.*s%02x%s", pt-fn, fn, i, pt);
		else
		    sprintf (cfn, "%s%02x", fn, i);	/* ?? */
		cfd = open (cfn, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (cfd < 0) {
		    fprintf (stderr, "%s: %s\n", cfn, strerror(errno));
		    continue;
		}

		/* fill with cropped portion */
		if (cropFITS(&cfim, &fim, rp->x, rp->y, rp->w, rp->h, buf) < 0)
		    fprintf (stderr, "Cropping %s: %s\n", cfn, buf);
		else if (writeFITS (cfd, &cfim, buf, 0) < 0)
		    fprintf (stderr, "Writing %s: %s\n", cfn, buf);
		else if (verbose)
		    printf ("%s: wrote\n", cfn);

		/* done with this new image */
		close (cfd);
		resetFImage (&cfim);
	    }

	    /* done with this source image */
	    close (fd);
	    resetFImage (&fim);
	}
}

/* add 1 to list of frects */
static void
addFRect (int x, int y, int w, int h)
{
	FITSRect *rp;

	if (frects)
	    frects= (FITSRect *) realloc (frects, (nfrects+1)*sizeof(FITSRect));
	else
	    frects= (FITSRect *) malloc (sizeof(FITSRect));
	if (!frects) {
	    fprintf (stderr, "No memory for another crop rectangle\n");
	    exit (1);
	}

	rp = &frects[nfrects++];
	rp->x = x;
	rp->y = y;
	rp->w = w;
	rp->h = h;
}

static void
cropOne (char *fn, CropAOI *aoip)
{
	char msg[1024];
	FImage fim, fout;
	double lx, ty, rx, by;
	int sw, sh;
	int x, y, w, h;
	int x0, y0, x1, y1;
	int fd;

	/* open the FITS file */
	fd = open (fn, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "%s: Can not open\n", fn);
	    return;
	}

	/* read it */
	initFImage (&fim);
	if (readFITS (fd, &fim, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, msg);
	    resetFImage (&fim);
	    close (fd);
	    return;
	}
	close (fd);

	/* get size to check bounds */
	if (getNAXIS (&fim, &sw, &sh, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, msg);
	    resetFImage (&fim);
	    return;
	}

	/* convert aoi to pixels -- allow for roundoff */
	if (RADec2xy (&fim, aoip->er, aoip->nd, &lx, &ty) < 0 ||
			    RADec2xy (&fim, aoip->wr, aoip->sd, &rx, &by) < 0) {
	    fprintf (stderr, "%s: can not get coords.\n", fn);
	    resetFImage (&fim);
	    return;
	}
	x0 = (int)floor(lx + 0.5);
	y0 = (int)floor(ty + 0.5);
	x1 = (int)floor(rx + 0.5);
	y1 = (int)floor(by + 0.5);
	x = x0;
	y = y0;
	w = x1-x0+1;
	h = y1-y0+1;

	/* sanity check */
	if (x < 0 || x+w > sw || y < 0 || y+h > sh) {
	    fprintf (stderr, "%s: bad AOI: x0=%d y0=%d x1=%d y1=%d\n", fn, 
								x0, y0, x1, y1);
	    resetFImage (&fim);
	    return;
	}

	/* make the cropped copy -- cropFITS checks its args carefully */
	initFImage (&fout);
	if (verbose) {
	    printf ("%s: lx=%g ty=%g rx=%g by=%g\n", fn, lx, ty,rx,by);
	    printf ("%s: x0=%d y0=%d x1=%d y1=%d\n", fn, x0, y0,x1,y1);
	    printf ("%s: cropFITS(%d,%d,%d,%d)\n", fn, x, y, w, h);
	}
	if (cropFITS (&fout, &fim, x, y, w, h, msg)) {
	    fprintf (stderr, "%s: Can not crop: %s\n", fn, msg);
	    resetFImage (&fim);
	    return;
	}

	/* open again for writing, with truncate */
	fd = open (fn, O_RDWR | O_TRUNC);
	if (fd < 0) {
	    fprintf (stderr, "%s: Can not open for writing.\n", fn);
	    resetFImage (&fim);
	    resetFImage (&fout);
	    return;
	}

	/* write new cropped image */
	if (writeFITS(fd, &fout, msg, 0) < 0) {
	    fprintf (stderr, "%s: Error writing cropped version: %s\n", fn,msg);
	    resetFImage (&fim);
	    resetFImage (&fout);
	    close (fd);
	    return;
	}

	/* finished */
	resetFImage (&fim);
	resetFImage (&fout);
	close (fd);
}

/* create a Region the full size of fip */
static Region
fitsRegion (FImage *fip)
{
	char msg[1024];
	XPoint p[4];
	int w, h;

	(void) getNAXIS (fip, &w, &h, msg);
	p[0].x = 0;   p[0].y = 0;
	p[1].x = w-1; p[1].y = 0;
	p[2].x = w-1; p[2].y = h-1;
	p[3].x = 0;   p[3].y = h-1;
	return (XPolygonRegion (p, 4, WindingRule));
}

/* create a Region containing all of fipi in the pixel coord system of fip0 */
static Region
fitsSubRegion (FImage *fip0, FImage *fipi)
{
	char msg[1024];
	XPoint p[4];
	int w, h;
	double r, d;
	double x, y;

	(void) getNAXIS (fipi, &w, &h, msg);
	(void) xy2RADec (fipi, 0.0, 0.0, &r, &d);
	(void) RADec2xy (fip0, r, d, &x, &y);
	p[0].x = (int)floor(x+.5); p[0].y = (int)floor(y+.5);
	(void) xy2RADec (fipi, w-1.0, 0.0, &r, &d);
	(void) RADec2xy (fip0, r, d, &x, &y);
	p[1].x = (int)floor(x+.5); p[1].y = (int)floor(y+.5);
	(void) xy2RADec (fipi, w-1.0, h-1.0, &r, &d);
	(void) RADec2xy (fip0, r, d, &x, &y);
	p[2].x = (int)floor(x+.5); p[2].y = (int)floor(y+.5);
	(void) xy2RADec (fipi, 0.0, h-1.0, &r, &d);
	(void) RADec2xy (fip0, r, d, &x, &y);
	p[3].x = (int)floor(x+.5); p[3].y = (int)floor(y+.5);

	/* find containing square */
	p[0].y = p[1].y = MAX(p[0].y ,p[1].y);
	p[1].x = p[2].x = MIN(p[1].x ,p[2].x);
	p[2].y = p[3].y = MIN(p[2].y ,p[3].y);
	p[3].x = p[0].x = MAX(p[3].x ,p[0].x);

	return (XPolygonRegion (p, 4, WindingRule));
}

/* convert region r, which is WRT fip, to a CropAOI */
static void
region2CropAOI (FImage *fip, Region reg, CropAOI *ap)
{
	XRectangle rec;
	double r, d;

	XClipBox (reg, &rec);

	(void) xy2RADec (fip, (double)rec.x, (double)rec.y, &r, &d);
	ap->er = r;
	ap->nd = d;
	(void) xy2RADec (fip, (double)(rec.x+rec.width-1),
					(double)(rec.y+rec.height-1), &r, &d);
	ap->wr = r;
	ap->sd = d;
}

static void
printRegion (char *msg, Region reg)
{
	XRectangle rec;

	XClipBox (reg, &rec);
	printf ("%s: %4d %4d %4d %4d\n", msg, rec.x, rec.y, rec.x+rec.width-1,
							    rec.y+rec.height-1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: crop.c,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $"};
