/* run hcomp and fitshdr on fits files.
 * similar to the old fcompress.csh but in C for easier porting.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "fits.h"

int scale = 1;
int rflag;
int verbose;

static void usage (char *p);
static int compressOne (char *fn);
static int alreadyCompressed (char *fn);

int
main (int ac, char *av[])
{
	char *progname = av[0];
	int nfiles = 0;
	char **fnames;
	char *str;
	int i;
	int nerrs;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    /* in the loop ac includes the current arg */
	    while (c = *++str)
		switch (c) {
		case 'f':	/* name of file containing a list of files */
		    if (ac < 2)
			usage (progname);
		    nfiles = readFilenames (*++av, &fnames);
		    if (nfiles == 0) {
			fprintf (stderr, "No files in %s\n", *av);
			usage (progname);
		    }
		    ac--;
		    break;
		case 'r':	/* remove the uncompressed file */
		    rflag = 1;
		    break;
		case 's':	/* set scale */
		    if (ac < 2)
			usage(progname);
		    scale = atoi (*++av);
		    ac--;
		    break;
		case 'v':	/* verbose */
		    verbose = 1;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0] */

	/* reuse ac/av from files read from files, if any */
	if (nfiles > 0) {
	    if (ac > 0) {
		fprintf (stderr, "Can not list files and use -f too.\n");
		exit(1);
	    }
	    ac = nfiles;
	    av = fnames;
	}

	nerrs = 0;
	for (i = 0; i < ac; i++)
	    if (compressOne (av[i]) < 0)
	        nerrs++;

	return (nerrs);
}

static void
usage (char *p)
{
	fprintf (stderr, "%s: [options] [*.fts]\n", p);
	fprintf (stderr, " -f file:  name of file of filenames to compress.\n");
	fprintf (stderr, " -r:       remove the uncompressed file.\n");
	fprintf (stderr, " -s scale: specify compression scale factor.\n");
	fprintf (stderr, "           default is 1, maximum lossless.\n");
	fprintf (stderr, " -v:       verbose\n");

	exit (1);
}

/* compress one file. return 0 if ok, else -1
 */
static int
compressOne (char *fn)
{
	char cmd[2048];
	char dir[2048];
	char base[128];
	char ext[32];
	char outfile[2048];
	
	if (verbose)
	    fprintf (stderr, "%s: reading\n", fn);

	/* decompose name and build up compressed name in outfile.
	 * also check for reasonable name and not already compressed.
	 */
	decomposeFN (fn, dir, base, ext);
	if (strcasecmp(ext,"fts") && strcasecmp (ext, "fits")) {
	    fprintf (stderr,"%s: extension must be .fts or .fits: %s\n", fn);
	    return (-1);
	}
	sprintf (outfile, "%s%s.fth", dir, base);
	if (alreadyCompressed(fn) < 0)
	    return (-1);

	/* add HCOM* headers to fn so they get into outfile, then take them
	 * back out again. all this because fitshdr can't edit compressed files.
	 */
	sprintf (cmd,"fitshdr -i HCOMSCAL %d -s HCOMSTAT HCOMPRSD %s",scale,fn);
	if (system (cmd)) {
	    fprintf (stderr, "%s: Could not add temp FITS header info\n", fn);
	    return (-1);
	}

	if (verbose)
	    fprintf (stderr, "%s: creating\n", outfile);

	sprintf (cmd, "hcomp -i fits -s %d < %s > %s", scale, fn,  outfile);
	if (system (cmd)) {
	    fprintf (stderr, "%s: Could not compress\n", fn);
	    remove (outfile);
	    return (-1);
	}

	sprintf (cmd, "fitshdr -d HCOMSCAL -d HCOMSTAT %s", fn);
	if (system (cmd)) {
	    fprintf (stderr, "%s: Could not remove FITS header info\n", fn);
	    return (-1);
	}

	if (rflag) {
	    if (verbose)
		fprintf (stderr, "%s: deleting\n", fn);
	    if (remove (fn) < 0) {
		fprintf (stderr, "remove %s: %s\n", strerror(errno));
		return (-1); 
	    }
	}

	return (0);
}

/* return 0 if image can be read ok and is not already compressed.
 * else -1.
 */
static int
alreadyCompressed (char *fn)
{
	FImage fimage, *fip = &fimage;
	char err[1024];
	int s, fd;

	fd = open (fn, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    return (-1);
	}

	initFImage (fip);
	s = readFITSHeader (fd, fip, err);
	(void) close (fd);
	if (s < 0) {
	    fprintf (stderr, "%s: %s\n", fn, err);
	    return (-1);
	}

	if (getStringFITS (fip, "HCOMSTAT", err) == 0 
					    && strcmp (err, "HCOMPRSD") == 0) {
	    fprintf (stderr, "%s: already compressed\n", fn);
	    return (-1);
	}

	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: fcompress.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
