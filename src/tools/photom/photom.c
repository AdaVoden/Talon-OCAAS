/* main program to photom.
 * see README for a description of usage.
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
#include "photom.h"

/* see photom.h */
int nstars;
FileInfo *files;
int nfiles;
StarDfn stardfn;
int verbose;
int wantfwhm;
int wantxy;

void usage (char *p);

int
main (ac, av)
int ac;
char *av[];
{
	char *progname = av[0];
	char *configfile = 0;
	int docrop = 0;
	char *str;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'c':	/* do cropping */
		    docrop++;
		    break;
		case 'f':	/* want fwhm */
		    wantfwhm++;
		    break;
		case 'x':	/* want xy */
		    wantxy++;
		    break;
		case 'v':	/* more verbosity */
		    verbose++;
		    break;
		default:
		    usage(progname);
		}
	}

	/* now there are ac remaining args starting at av[0].
	 * there should be one -- the config file name.
	 */
	if (ac == 1)
	    configfile = av[0];
	else
	    usage (progname);

	/* read the config file.
	 * this creates and fills files[] with all sl, dx and dy fields set.
	 * it also establishes nfiles and nstars.
	 */
	readConfigFile (configfile);

	/* do the photometry */
	doAllImages();

	/* do the final cropping, if desired */
	if (docrop)
	    doCropping();

	return (0);
}

void
usage (p)
char *p;
{
	fprintf (stderr, "%s [-vc] config_file\n", p);
	fprintf (stderr, "  -c: crop files to bounding box *IN PLACE*\n");
	fprintf (stderr, "  -f: include FWHM geo mean and ratio in report\n");
	fprintf (stderr, "  -x: include X and Y image location in report\n");
	fprintf (stderr, "  -v: verbose\n");
	exit (1);
}

/* read the given FITS file into fip.
 * just read the header or the whole thing.
 * return 0 if ok else -1.
 */
int
readFITSfile (fn, justheader, fip)
char *fn;
int justheader;
FImage *fip;
{
	char errmsg[1024];
	int fd;

#ifdef _WIN32
	fd = open (fn, O_RDONLY|O_BINARY);
#else
	fd = open (fn, O_RDONLY);
#endif
	if (fd < 0) {
	    perror (fn);
	    return (-1);
	}

	if (justheader) {
	    if (readFITSHeader (fd, fip, errmsg) < 0) {
		fprintf (stderr, "Error reading %s: %s\n", fn, errmsg);
		return (-1);
	    }
	} else {
	    if (readFITS (fd, fip, errmsg) < 0) {
		fprintf (stderr, "Error reading %s: %s\n", fn, errmsg);
		return (-1);
	    }
	}

	(void) close (fd);

	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: photom.c,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $"};
