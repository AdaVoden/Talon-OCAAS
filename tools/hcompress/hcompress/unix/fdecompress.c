/* run hdecomp and fitshdr on fits files.
 * similar to the old fdecompress.csh but in C for easier porting.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include "configfile.h"	/* for readFilenames() */

int rflag;
int verbose;

static void usage (char *p);
static int decompressOne (char *fn);

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
		case 'f':	/* list of files */
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
	    if (decompressOne (av[i]) < 0)
		nerrs++;

	return (nerrs);
}

static void
usage (char *p)
{
	fprintf(stderr,"%s: [options] [*.fth]\n", p);
	fprintf(stderr," -f file:  name of file of filenames to decompress.\n");
	fprintf(stderr," -r:       remove the compressed file.\n");
	fprintf(stderr," -v:       verbose\n");

	exit (1);
}

/* decompress one file. return 0 if ok, else -1
 */
static int
decompressOne (char *fn)
{
	char cmd[2048];
	char dir[2048];
	char base[128];
	char ext[32];
	char outfile[2048];
	
	if (verbose)
	    fprintf (stderr, "%s: reading\n", fn);

	/* decompose and build up compressed name */
	decomposeFN (fn, dir, base, ext);
	if (strcasecmp (ext, "fth")) {
	    fprintf (stderr, "%s: does not end in .fth\n", fn);
	    return (-1);
	}
	sprintf (outfile, "%s%s.fts", dir, base);

	if (verbose)
	    fprintf (stderr, "%s: creating\n", outfile);

	sprintf (cmd, "hdecomp < %s > %s", fn, outfile);
	if (system (cmd)) {
	    fprintf (stderr, "%s: Could not decompress\n", fn);
	    remove (outfile);
	    return (-1);
	}

	sprintf (cmd, "fitshdr -s HCOMSTAT UNHCOMP %s", outfile);
	if (system (cmd)) {
	    fprintf (stderr, "%s: Could not add FITS header info\n", outfile);
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

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: fdecompress.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
