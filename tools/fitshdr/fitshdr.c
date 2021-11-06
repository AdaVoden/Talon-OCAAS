/* utility to manipulate fits headers.
 * if no files are given, we read stdin and write updated version to stdout.
 * if files are given we make the mods to each IN PLACE.
 * we only support INTEGER*16 data.
 * 18 Sep 96 add -n option to prefix each line with a filename
 * 22 Jul 96 Just read header if running readonly
 *  4 Nov 98 do nothing gracefully if give empty -f file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "strops.h"
#include "configfile.h"
#include "fits.h"

typedef struct {
    FITSRow *name;
    FITSRow *valu;
    int n;
} Field;

static Field newi;
static Field newl;
static Field newr;
static Field news;
static Field newc;
static Field del;

static void usage(char *p);
static void newField (Field *fp, char *n, char *v);
static FITSRow *growFR (FITSRow *old, int newn);
static void processFile (int fdi, char *namei, int fdo);
static void printHdr (char *fn, FImage *fip);

static int printhdr;
static int readonly;
static int prefixfn;

int
main (ac, av)
int ac;
char *av[];
{
	char *progname = av[0];
	int nfiles = -1;
	char **fnames;
	char *str;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    /* in the loop ac includes the current arg */
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'd':	/* delete field */
		    if (ac < 2)
			usage(progname);
		    newField (&del, av[1], "");
		    ac -= 1;
		    av += 1;
		    break;
		case 'i':	/* add integer field */
		    if (ac < 3)
			usage (progname);
		    newField (&newi, av[1], av[2]);
		    ac -= 2;
		    av += 2;
		    break;
		case 'l':	/* add logical field */
		    if (ac < 3)
			usage (progname);
		    newField (&newl, av[1], av[2]);
		    ac -= 2;
		    av += 2;
		    break;
		case 'r':	/* add real field */
		    if (ac < 3)
			usage (progname);
		    newField (&newr, av[1], av[2]);
		    ac -= 2;
		    av += 2;
		    break;
		case 's':	/* add string field */
		    if (ac < 3)
			usage (progname);
		    newField (&news, av[1], av[2]);
		    ac -= 2;
		    av += 2;
		    break;
		case 'c':	/* add comment field */
		    if (ac < 2)
			usage (progname);
		    newField (&newc, "COMMENT", av[1]);
		    ac -= 1;
		    av += 1;
		    break;
		case 'P':	/* prefix filename */
		    prefixfn++;
		    /* FALLTHRU */
		case 'p':	/* print the resulting header */
		    printhdr++;
		    break;
		case 'f':	/* names of files are in a file */
		    if (ac < 2)
			usage (progname);
		    nfiles = readFilenames (av[1], &fnames);
		    ac -= 1;
		    av += 1;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0] */

	/* set up ac/av to be the list of files, even if from a file */
	if (nfiles >= 0) {
	    if (ac > 0) {
		fprintf(stderr, "Can not name files and use -f too\n");
		usage (progname);
	    }
	    ac = nfiles;
	    av = fnames;
	}

	/* if no edits just print header and open files for readonly */
	if (!newi.n && !newl.n && !newr.n && !news.n && !newc.n && !del.n) {
	    readonly = 1;
	    printhdr = 1;
	}

	if (nfiles >= 0 || ac > 0) {
	    for (; ac > 0; ac--, av++) {
		int ifd, ofd;
		char *fn = av[0];

		ifd = open (fn, O_RDONLY);
		if (ifd < 0) {
		    perror (fn);
		    continue;
		}
		ofd = open (fn, readonly ? O_RDONLY : O_WRONLY);
		if (ofd < 0) {
		    perror (fn);
		    (void) close (ifd);
		    continue;
		}

		processFile (ifd, fn, ofd);
		(void) close (ifd);
		(void) close (ofd);
	    }
	} else {
	    processFile (0, "stdin", 1);
	}
	return (0);
}

static void
usage(p)
char *p;
{
	FILE *f = stderr;

	fprintf (f, "Usage: %s [options] [files ...]\n", p);
	fprintf (f, "Purpose: print and manipulate FITS headers.\n");
	fprintf (f, "Options:\n");
	fprintf (f, "  -f name:       name of file containing file names, one per line\n");
	fprintf (f, "  -d name:       delete field of any type of name\n");
	fprintf (f, "  -i name value: add/replace int field name/value\n");
	fprintf (f, "  -l name value: add/replace logical field name/{0,1}\n");
	fprintf (f, "  -r name value: add/replace real field name/value\n");
	fprintf (f, "  -s name value: add/replace string field name/value\n");
	fprintf (f, "  -c value:      add COMMENT field value\n");
	fprintf (f, "  -p:            print [new] header (default)\n");
	fprintf (f, "  -P:            same as -p and prefix each line with filename\n");

	exit (1);
}

/* add a new name/value to the given Field */
static void
newField (fp, n, v)
Field *fp;
char *n;
char *v;
{
	int newn = fp->n + 1;

	fp->name = growFR (fp->name, newn);
	fp->valu = growFR (fp->valu, newn);

	sprintf (fp->name[fp->n], "%.*s", (int)sizeof(FITSRow)-1, n);
	sprintf (fp->valu[fp->n], "%.*s", (int)sizeof(FITSRow)-1, v);
	fp->n = newn;
}

static FITSRow *
growFR (FITSRow *old, int newn)
{
	int sz = sizeof(FITSRow);
	char *newm;

	if (!old)
	    newm = malloc (newn*sz);
	else
	    newm = realloc ((void *)old, newn*sz);

	if (!newm) {
	    fprintf (stderr, "No more memory\n");
	    exit (1);
	}

	return ((FITSRow *)newm);
}

static void
processFile (fdi, namei, fdo)
int fdi;
char *namei;
int fdo;
{
	FImage fimage, *fip = &fimage;
	char errmsg[1024];
	int nvar;
	int i;

	initFImage (fip);

	/* read header, or whole thing if from stdin which we truely copy */
	if (fdi == 0) {
	    if (readFITS (fdi, fip, errmsg) < 0) {
		fprintf (stderr, "%s: %s\n", namei, errmsg);
		return;
	    }
	} else {
	    if (readFITSHeader (fdi, fip, errmsg) < 0) {
		fprintf (stderr, "%s: %s\n", namei, errmsg);
		return;
	    }
	}

	/* save initial size of header */
	nvar = fip->nvar;

	/* do the deletions -- replacements are done in-place */
	for (i = 0; i < del.n; i++)
	    (void) delFImageVar (fip, del.name[i]);

	/* add the int values */
	for (i = 0; i < newi.n; i++)
	    setIntFITS (fip, newi.name[i], atoi(newi.valu[i]), NULL);

	/* add the logical values */
	for (i = 0; i < newl.n; i++)
	    setLogicalFITS (fip, newl.name[i], atoi(newl.valu[i]), NULL);

	/* add the real values */
	for (i = 0; i < newr.n; i++)
	    setRealFITS (fip, newr.name[i], atof(newr.valu[i]), 16, NULL);

	/* add the string values */
	for (i = 0; i < news.n; i++)
	    setStringFITS (fip, news.name[i], news.valu[i], NULL);

	/* add the comment values */
	for (i = 0; i < newc.n; i++)
	    setCommentFITS (fip, newc.name[i], newc.valu[i]);

	/* print header if desired */
	if (printhdr)
	    printHdr (basenm(namei), fip);

	/* unless readonly, write header unless diff size or true copy */
	if (!readonly) {
	    if (fdi == 0) {
		/* already read whole file, write whole copy */
		if (writeFITS (fdo, fip, errmsg, 0) < 0)
		    fprintf (stderr, "%s: Erroring writing: %s\n",namei,errmsg);
	    } else if (nvar/FITS_HROWS == fip->nvar/FITS_HROWS) {
		/* same size header, so just rewrite that part */
		if (writeFITSHeader (fip, fdo, errmsg) < 0)
		    fprintf (stderr, "%s: Erroring rewriting header: %s\n",
								namei, errmsg);
	    } else {
		/* diff size hdr and don't have pixels: read all, write all */
		FImage whole, *wp = &whole;
		initFImage (wp);
		if (lseek (fdi, 0L, SEEK_SET) == (off_t)-1) {
		    fprintf (stderr, "%s: seek: %s", namei, strerror(errno));
		} else if (readFITS (fdi, wp, errmsg) < 0) {
		    fprintf (stderr, "%s: %s\n", namei, errmsg);
		} else {
		    copyFITSHeader (wp, fip);
		    if (wp->nvar < nvar)
			(void) ftruncate (fdo, 0); /* don't fail if can't */
		    if (writeFITS (fdo, wp, errmsg, 0) < 0)
			fprintf (stderr, "%s: Erroring rewriting: %s\n", namei,
								    errmsg);
		}
		resetFImage (wp);
	    }
	}

	/* done */
	resetFImage (fip);
}

static void
printHdr (fn, fip)
char *fn;
FImage *fip;
{
	int i, j;
	char buf[128];

	for (i = 0; i < fip->nvar; i++) {
	    memcpy (buf, fip->var[i], FITS_HCOLS);
	    for (j = FITS_HCOLS; --j >= 0; )
		if (buf[j] == ' ')
		    buf[j] = '\0';
		else
		    break;
	    if (prefixfn)
		printf ("%-14s ", fn);	/* N.B. 14 is important to cdrcv et al*/
	    printf ("%.80s\n", buf);
	}
	printf ("\n");
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: fitshdr.c,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $"};
