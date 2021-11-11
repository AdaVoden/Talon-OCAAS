/* drive several misc fits filters */

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
#include "strops.h"
#include "configfile.h"

typedef int (*IPFP)(FImage *fip, FImage *tp,  int);

static void usage(char *progname);
static int doIPFunc (char *fn, IPFP fp, int arg, char *history);
static int openImage (char *name, FImage *fip, char msg[]);

typedef struct {
    char sw;		/* command-line setting */
    IPFP fp;		/* actual processing function */
    char *history;	/* FITS HISTORY string format, with one %d */
} IPFunc;

static IPFunc ipfuncs[] = {
    {'m', medianFilter, "Median filter of size %d"},
    {'l', flatField, "Apply order-%d flat field"}
};
#define	NIPF	(sizeof(ipfuncs)/sizeof(ipfuncs[0]))


int
main (int ac, char *av[])
{
	char *progname = av[0];
	int nfiles = 0;
	char **fnames;
	IPFunc *ipfp = NULL;
	char history[1024];
	int arg = 0;
	int nbad;
	int i;

	/* crack arguments */
	for (av++; --ac > 0 && **av == '-'; av++) {
	    char c, *str = *av;
	    while ((c = *++str)) {
		if (c == 'f') {
		    /* file of filenames */
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
		} else if (c == 'h') {
		    usage (progname);
		} else {
		    /* decide filter selection */
		    if (ipfp) {
			fprintf (stderr, "Only one filter at a time\n");
			usage (progname);
		    }
		    for (i = 0; i < NIPF; i++) {
			if (c == ipfuncs[i].sw) {
			    if (ac < 2) {
				fprintf (stderr, "-%c requires arg\n", c);
				usage (progname);
			    }
			    arg = atoi (*++av);
			    ac--;
			    ipfp = &ipfuncs[i];
			    break;
			}
		    }
		}
	    }
	}

	/* now there are ac args left starting at av[0] */

	/* confirm legal selection */
	if (ipfp == NULL) {
	    fprintf (stderr, "No valid filter selection\n");
	    usage (progname);
	}

	/* substitute files from file for ac/av if -f option was used */
	if (nfiles > 0) {
	    if (ac > 0) {
		fprintf (stderr, "Can not give files and use -f too.\n");
		exit (1);
	    }
	    ac = nfiles;
	    av = fnames;
	}


	/* do it */
	nbad = 0;
	sprintf (history, ipfp->history, arg);
	while (ac-- > 0)
	    if (doIPFunc (*av++, ipfp->fp, arg, history) < 0)
		nbad++;

	return (nbad);
}

static void
usage(char *progname)
{
	FILE *fp = stderr;

	fprintf(fp,"%s: usage: [options] file.fts ...\n", progname);
	fprintf(fp,"Purpose: perform a filter on given fits files IN PLACE.\n");
	fprintf(fp,"  -f file: name of file containing filenames\n");
	fprintf(fp,"  -l n   : apply order-n 2d polynomial flat-field\n");
	fprintf(fp,"  -m s   : median filter of size s\n");
	exit (1);
}

static int
doIPFunc (char *fn, IPFP fp, int arg, char history[])
{
	char *bn = basenm(fn);
	FImage from, *fip = &from;
	FImage to, *tip = &to;
	char msg[1024];	
	int ok;
	int fd;

	initFImage (fip);
	initFImage (tip);

	fd = openImage (fn, fip, msg);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", bn, msg);
	    return (-1);
	}

	copyFITS (tip, fip);

	if ((*fp) (fip, tip, arg) < 0) {
	    fprintf (stderr, "%s: filter failed\n", bn);
	    return (-1);
	}

	setCommentFITS (tip, "HISTORY", history);

	if (lseek (fd, 0L, SEEK_SET) == (off_t)(-1)) {
	    fprintf (stderr, "%s: %s\n", bn, strerror(errno));
	    ok = -1;
	} else {
	    ok = writeFITS (fd, tip, msg, 0);
	    if (ok < 0)
		fprintf (stderr, "%s: %s\n", bn, msg);
	}

	(void) close (fd);
	resetFImage (fip);
	resetFImage (tip);

	return (ok);
}

/* open the given FITS file for read/write.
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

	return (fd);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: fitsfilter.c,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $"};
