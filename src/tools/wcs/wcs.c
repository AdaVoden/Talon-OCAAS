/* tools/wcs/wcs.c modified to call setWCSFITSD instead of setWCSFITS
 * and in verbose mode to print new astrometric coefficients if they are
 * calculated
 *
 * Last update DJA 020917
 * Merged STO 021001
 * Switchable ip.cfg file, STO 021227
 *
 * New features enabled by defining USE_DISTANCE_METHOD as 1 in wcs.h
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
#include "fieldstar.h"
#include "telenv.h"
#include "strops.h"
#include "configfile.h"
#include "wcs.h"

static void usage(char *prognam);
static void delPos (char *name);
static int calPos (char *name);
static int openImage (char *name, int rw, FImage *fip, char msg[]);
static void checkEquinox (FImage *fip);
#if USE_DISTANCE_VERSION
int setWCSFITSD (FImage *fip, int wantusno, double hunt, int (*bfp)(),
                 int verbose, char msg[]);
int delWCSFITSD (FImage *fip, int verbose);
int checkWCSFITSD (FImage *fip, int verbose);
#endif
static int verbose = 0;		/* verbose/debugging flag */
static int writeheader = 0;	/* write header fields; else read-only */
static int overwrite = 0;	/* allow overwriting if fields already exist */
static int delete = 0;		/* delete the WCD header fields */
static int useusno = 0;		/* use USNO catalog too */

#define	HUNTRAD	degrad(1.0)	/* default GSC search hunt radius, rads */
static double huntrad = HUNTRAD;

static char *chpath_default = "archive/catalogs/gsc";
static char *uspath_default = "archive/catalogs/usno";

static char *cdpath;
static char *chpath;
static char *uspath;

int
main (int ac, char *av[])
{
	char *progname = av[0];
	int nfiles = 0;
	char **fnames;
	char *str;
	int nbad = 0;
	char oldIpCfg[256];

	// Save existing IP.CFG.... be sure to restore before any exit!!	
	strcpy(oldIpCfg,getCurrentIpCfgPath());

	/* set up defaults */
	chpath = chpath_default;
	uspath = uspath_default;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str))
		switch (c) {
		case 'c':	/* alternate cache dir */
		    if (ac < 2) {
			fprintf (stderr, "-c requires cache path\n");
			setIpCfgPath(oldIpCfg);
			usage (progname);
		    }
		    chpath = *++av;
		    ac--;
		    break;
		case 'd':	/* delete WCS headers */
		    delete++;
		    break;
		case 'f':	/* file of filenames */
		    if (ac < 2) {
			fprintf (stderr, "-f requires filename of files\n");
			setIpCfgPath(oldIpCfg);
			usage (progname);
		    }
		    nfiles = readFilenames (*++av, &fnames);
		    if (nfiles == 0) {
			fprintf (stderr, "No files found in %s\n", *av);
			setIpCfgPath(oldIpCfg);
			exit(1);
		    }
		    ac--;
		    break;
		case 'n':	/* alternate USNO dir */
		    if (ac < 2) {
			fprintf (stderr, "-n requires USNO path\n");
			setIpCfgPath(oldIpCfg);
			usage (progname);
		    }
		    uspath = *++av;
		    ac--;
		    break;
		case 'o':	/* allow overwriting existing WCS headers */
		    overwrite++;
		    break;
		case 'r':	/* alternate cdrom dir */
		    if (ac < 2) {
			fprintf (stderr, "-r requires cdrom path\n");
			setIpCfgPath(oldIpCfg);
			usage (progname);
		    }
		    cdpath = *++av;
		    ac--;
		    break;
		case '2':	/* use USNO SA too */
		    useusno++;
		    break;
		case 'u':	/* hunting radius */
		    if (ac < 2) {
			fprintf (stderr, "-u requires hunt radius, rads\n");
			setIpCfgPath(oldIpCfg);
			usage (progname);
		    }
		    huntrad = degrad(atof(*++av));
		    ac--;
		    break;
		case 'v':	/* more verbosity */
		    verbose++;
		    break;
		case 'w':	/* update the fields */
		    writeheader++;
		    break;
		case 'i':	/* alternate ip.cfg */
			if(ac < 2) {
				fprintf(stderr, "-i requires ip.cfg pathname\n");
				setIpCfgPath(oldIpCfg);
				usage(progname);
			}
			setIpCfgPath(*++av);
			ac--;
			break;
		default:
			setIpCfgPath(oldIpCfg);
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
		setIpCfgPath(oldIpCfg);
	    usage (progname);
	}

	if (delete && !writeheader) {
	    fprintf (stderr, "%s: -d also requires -w.\n", progname);
		setIpCfgPath(oldIpCfg);
	    usage(progname);
	}

	if (delete && overwrite) {
	    fprintf (stderr, "%s: do not use -o and -d together.\n", progname);
		setIpCfgPath(oldIpCfg);
	    usage(progname);
	}

	if (!delete && !writeheader && !verbose) {
	    fprintf (stderr, "%s: need at least one of -dwv.\n", progname);
		setIpCfgPath(oldIpCfg);
	    usage(progname);
	}

	if (delete)
	    while (ac-- > 0) {
		char *fn = *av++;
		if (verbose)
		    printf ("%s:\n", fn);
		delPos (fn);
		if (verbose)
		    printf ("\n");
	    }
	else {
	    /* set up search paths then go */
	    char telchpath[1024];
	    char teluspath[1024];
	    char msg[1024];

	    telfixpath (telchpath, chpath);
	    if (GSCSetup (cdpath, telchpath, msg) < 0) {
		fprintf (stderr, "GSC: %s\n", msg);
		setIpCfgPath(oldIpCfg);
		exit (1);
	    }

	    telfixpath (teluspath, uspath);
	    if (USNOSetup (teluspath, 0, msg) < 0 && verbose)
		printf ("USNO: %s\n", msg);	/* no fatal */

	    while (ac-- > 0) {
		char *fn = *av++;
		if (verbose)
		    printf ("%s:\n", fn);
		if (calPos (fn) < 0)
		    nbad++;
		if (verbose)
		    printf ("\n");
	    }
	}
	
	setIpCfgPath(oldIpCfg);

	return (nbad);
}

static void
usage(char *progname)
{
	FILE *fp = stderr;

	fprintf(fp,"%s: usage: [options] [file.fts ...]\n", progname);
	fprintf(fp,"  -r dir:  use GSC from cdrom at dir (default is off)\n");
	fprintf(fp,"  -c dir:  alternate GSC cache directory (default is %s)\n",
							    chpath_default);
	fprintf(fp,"  -n dir:  alternate USNO directory (default is %s)\n",
							    uspath_default);
	fprintf(fp,"  -v:      verbose\n");
	fprintf(fp,"  -i path: alternate ip.cfg file path (default is %s)\n",
								getCurrentIpCfgPath());
	fprintf(fp,
#if USE_DISTANCE_METHOD
	"  -o:      allow overwriting any existing WCS/astrometry fields\n");
#else	
	"  -o:      allow overwriting any existing WCS fields\n");
#endif
	fprintf(fp,"  -w:      write header (default is read-only)\n");
	fprintf(fp,
#if USE_DISTANCE_METHOD
	"  -d:      delete any WCS/astrometry header fields (requires -w)\n");
#else	
	"  -d:      delete any WCS header fields (requires -w)\n");
#endif	
	fprintf(fp,"  -s:      use USNO SA catalog in addition to GSC\n");
	fprintf(fp,"  -u rad:  hunt radius, degrees (default is %g)\n",
							    raddeg(HUNTRAD));
	fprintf(fp,"  -f file: name of file containing filenames.\n");
	exit (1);
}

static void
delPos (char *name)
{
	char *bn = basenm(name);
	FImage fimage;		/* FITS image */
	char msg[1024];		/* error messages */
	int fd;

	initFImage (&fimage);

	fd = openImage (name, writeheader, &fimage, msg);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", bn, msg);
	    return;
	}

	if (delWCSFITS (&fimage, verbose) < 0) {
	    if (verbose)
		printf ("%s: no WCS fields found -- file unchanged.\n", bn);
	} else if (lseek (fd, 0L, 0) < 0) {	/* rewind fd */
	    fprintf (stderr, "%s: Can not rewind: %s\n", bn, strerror(errno));
	} else if (writeFITS (fd, &fimage, msg, 0) < 0) {
	    fprintf (stderr, "%s: %s\n", bn, msg);
	} else if (verbose)
	    printf ("%s: rewritten successfully.\n", bn);


	resetFImage (&fimage);
	(void) close (fd);
}

static int
calPos (char *name)
{
	char *bn = basenm(name);
	FImage fimage;		/* FITS image */
	char msg[1024];		/* error messages */
	int ret;
	int fd;

	initFImage (&fimage);

	/* open the image */
	fd = openImage (name, writeheader, &fimage, msg);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", bn, msg);
	    return (-1);
	}

	ret = 0;

	/* print existing WCS headers and check for permission to overwrite */
	if (checkWCSFITS (&fimage, verbose) == 0 && writeheader && !overwrite) {
	    fprintf (stderr,
#if USE_DISTANCE_METHOD	
	        "%s: use -o to overwrite existing WCS/astrometry fields.\n", bn);
#else
		    "%s: use -o to overwrite existing WCS fields.\n", bn);
#endif		
	    ret = -1;
	    goto out;
	}

	if (setWCSFITS (&fimage, useusno, huntrad, 0, verbose, msg) < 0) {
	    fprintf (stderr, "%s: Can not compute WCS: %s\n", bn, msg);
	    ret = -1;
	    goto out;
	}

	if (verbose)
	    (void) checkWCSFITS (&fimage, verbose);	/* print new ones */

	if (writeheader) {
	    /* insure there is at least one of EPOCH or EQUINOX fields */
	    checkEquinox (&fimage);

	    if (lseek (fd, 0L, 0) < 0)	/* rewind fd */
		fprintf (stderr, "%s: Can not rewind: %s\n", bn,
							strerror(errno));
	    else if (writeFITS (fd, &fimage, msg, 0) < 0)
		fprintf (stderr, "%s: %s\n", bn, msg);
	    else if (verbose)
		printf ("%s: rewritten successfully.\n", bn);
        } else if (verbose)
	    printf ("%s: no -w -- file unchanged.\n", bn);

    out:

	resetFImage (&fimage);
	(void) close (fd);

	return (ret);
}

/* open the given FITS file for read/write.
 * if ok return file des, else return -1 with excuse in msg[].
 */
static int
openImage (char *name, int rw, FImage *fip, char msg[])
{
	int fd;

	fd = open (name, rw ? O_RDWR : O_RDONLY);
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

/* insure fip has at least one of EPOCH or EQUINOX.
 * if neither, add both set to 2000.0.
 */
static void
checkEquinox (fip)
FImage *fip;
{
	static char ep[] = "EPOCH";
	static char eq[] = "EQUINOX";
	double p, q;

	if (getRealFITS (fip, ep, &p) < 0 && getRealFITS (fip, eq, &q) < 0) {
	    setRealFITS (fip, ep, 2000.0, 10, NULL);
	    setRealFITS (fip, eq, 2000.0, 10, NULL);
	}
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: wcs.c,v $ $Date: 2003/04/15 20:48:46 $ $Revision: 1.1.1.1 $ $Name:  $"};
