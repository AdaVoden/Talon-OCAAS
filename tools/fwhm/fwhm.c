/* compute and add FWHMH/FWHMV/FWHMVS/FWHMHS FITS header fields
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
#include "telenv.h"
#include "strops.h"
#include "configfile.h"

static void usage(char *prognam);
static void delFWHM (char *name);
static int addFWHM (char *name);
static int chkFWHM (FImage *fip, int prtoo);
static int openImage (char *name, int rw, FImage *fip, char msg[]);

static int verbose = 0;		/* verbose/debugging flag */
static int writeheader = 0;	/* write header fields; else read-only */
static int overwrite = 0;	/* allow overwriting if fields already exist */
static int delete = 0;		/* delete the WCD header fields */
static int arcsecs = 0;		/* print in arc seconds if can */

int
main (int ac, char *av[])
{
	char *progname = av[0];
	int nfiles = 0;
	char **fnames;
	char *str;
	int nbad = 0;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str))
		switch (c) {
		case 'a':	/* print in arc seconds if can */
		    arcsecs++;
		    break;
		case 'd':	/* delete FWHM headers */
		    delete++;
		    break;
		case 'f':	/* file of filenames */
		    if (ac < 2) {
			fprintf (stderr, "-f requires filename of files\n");
			usage (progname);
		    }
		    nfiles = readFilenames (*++av, &fnames);
		    if (nfiles == 0) {
			fprintf (stderr, "No files found in %s\n", *av);
			exit(1);
		    }
		    ac--;
		    break;
		case 'o':	/* allow overwriting existing FWHM headers */
		    overwrite++;
		    break;
		case 'v':	/* more verbosity */
		    verbose++;
		    break;
		case 'w':	/* update the fields */
		    writeheader++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining file names starting at av[0] */

	/* substitute files from file for ac/av if -f option was used */
	if (nfiles > 0) {
	    if (ac > 0) {
		fprintf (stderr, "Can not give files and use -f too.\n");
		exit (1);
	    }
	    ac = nfiles;
	    av = fnames;
	}

	if (ac == 0) {
	    fprintf (stderr, "No files or file of files\n");
	    usage (progname);
	}

	if (delete && !writeheader) {
	    fprintf (stderr, "%s: -d also requires -w.\n", progname);
	    usage(progname);
	}

	if (delete && overwrite) {
	    fprintf (stderr, "%s: do not use -o and -d together.\n", progname);
	    usage(progname);
	}

	if (!delete && !writeheader && !verbose) {
	    fprintf (stderr, "%s: need at least one of -dwv.\n", progname);
	    usage(progname);
	}

	if (delete)
	    while (ac-- > 0)
		delFWHM (*av++);
	else {
	    while (ac-- > 0)
		if (addFWHM (*av++) < 0)
		    nbad++;
	}

	return (nbad > 0 ? 1 : 0);
}

static void
usage(char *progname)
{
	FILE *fp = stderr;

	fprintf(fp,"%s: usage: [options] [file.fts ...]\n", progname);
	fprintf(fp,"  -a:      print in arc seconds if can, else pixels\n");
	fprintf(fp,"  -v:      verbose\n");
	fprintf(fp,"  -o:      allow overwriting any existing FWHM fields\n");
	fprintf(fp,"  -w:      write header (default is read-only)\n");
	fprintf(fp,"  -d:      delete any FWHM header fields (requires -w)\n");
	fprintf(fp,"  -f file: name of file containing filenames.\n");
	exit (1);
}

/* delete any and all of the FWHM family of FITS fields */
static void
delFWHM (char *name)
{
	FImage fimage, *fip = &fimage;
	char *bn = basenm(name);
	char msg[1024];
	int fd;
	int n;

	initFImage (fip);

	fd = openImage (name, writeheader, fip, msg);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", bn, msg);
	    return;
	}

	n = 0;
	if (delFImageVar(fip, "FWHMH") == 0)
	    n++;
	if (delFImageVar(fip, "FWHMV") == 0)
	    n++;
	if (delFImageVar(fip, "FWHMHS") == 0)
	    n++;
	if (delFImageVar(fip, "FWHMVS") == 0)
	    n++;

	if (n == 0) {
	    if (verbose)
		printf ("%s: no FWHM fields found -- file unchanged.\n", bn);
	} else if (lseek (fd, 0L, 0) < 0) {	/* rewind fd */
	    fprintf (stderr, "%s: Can not rewind: %s\n", bn, strerror(errno));
	} else if (writeFITS (fd, fip, msg, 0) < 0) {
	    fprintf (stderr, "%s: %s\n", bn, msg);
	} else if (verbose)
	    printf ("%s: rewritten successfully.\n", bn);


	resetFImage (fip);
	(void) close (fd);
}

static int
addFWHM (char *name)
{
	FImage fimage, *fip = &fimage;
	char *bn = basenm(name);
	char msg[1024];
	int ret;
	int fd;

	initFImage (fip);

	/* open the image */
	fd = openImage (name, writeheader, fip, msg);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", bn, msg);
	    return (-1);
	}

	ret = 0;

	/* check for permission to overwrite */
	if (chkFWHM (fip, 0) == 0 && writeheader && !overwrite) {
	    fprintf (stderr, "%s: use -o to overwrite existing FWHM fields.\n",
									bn);
	    ret = -1;
	    goto out;
	}

	/* compute and set the fields */
	if (setFWHMFITS (fip, msg) < 0) {
	    fprintf (stderr, "%s: Can not compute FWHM: %s\n", bn, msg);
	    ret = -1;
	    goto out;
	}

	if (verbose) {
	    printf ("%s: ", bn);
	    (void) chkFWHM (fip, 1);	/* print new ones */
	}

	if (writeheader) {
	    if (lseek (fd, 0L, 0) < 0)	/* rewind fd */
		fprintf (stderr, "%s: Can not rewind: %s\n", bn,
							strerror(errno));
	    else if (writeFITS (fd, fip, msg, 0) < 0)
		fprintf (stderr, "%s: %s\n", bn, msg);
	    else if (verbose)
		printf ("%s: rewritten successfully.\n", bn);
        } else if (verbose)
	    printf ("%s: no -w -- file unchanged.\n", bn);

    out:

	resetFImage (fip);
	(void) close (fd);

	return (ret);
}

/* return 0 if _any_ are present, else -1
 * also print out if prtoo.
 */
static int
chkFWHM (FImage *fip, int prtoo)
{
	double cd1, cd2;
	double tmp;
	int cdok;
	int n;

	/* see whether we can compute arc seconds */
	cdok = getRealFITS (fip, "CDELT1", &cd1) == 0 &&
				    getRealFITS (fip, "CDELT2", &cd2) == 0;
	if (cdok) {
	    cd1 = fabs(cd1)*3600.0;
	    cd2 = fabs(cd2)*3600.0;
	} else {
	    cd1 = cd2 = 1.0;
	}

	if (prtoo)
	    printf ("FWHM, %s:", cdok ? "arcsecs" : "pixels");

	n = 0;
	if (getRealFITS (fip, "FWHMH", &tmp) == 0) {
	    if (prtoo)
		printf (" H=%4.1f", tmp*cd1);
	    n++;
	}
	if (getRealFITS (fip, "FWHMV", &tmp) == 0) {
	    if (prtoo)
		printf (" V=%4.1f", tmp*cd2);
	    n++;
	}
	if (getRealFITS (fip, "FWHMHS", &tmp) == 0) {
	    if (prtoo)
		printf (" HS=%4.1f", tmp*cd1);
	    n++;
	}
	if (getRealFITS (fip, "FWHMVS", &tmp) == 0) {
	    if (prtoo)
		printf (" VS=%4.1f", tmp*cd2);
	    n++;
	}

	if (prtoo)
	    printf ("\n");

	return (n > 0 ? 0 : -1);
}

/* open the given FITS file for read/write.
 * if ok return file des, else return -1 with excuse in msg[].
 */
static int
openImage (char *name, int rw, FImage *fip, char msg[])
{
	int fd;

#ifdef _WIN32
	fd = open (name, (rw ? O_RDWR : O_RDONLY) | O_BINARY);
#else
	fd = open (name, rw ? O_RDWR : O_RDONLY);
#endif
	if (fd < 0) {
	    strcpy (msg, strerror (errno));
	    return (-1);
	}

	if (readFITS (fd, fip, msg) < 0) {
	    (void) close (fd);
	    return (-1);
	}

	return (fd);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: fwhm.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
