/* search a set of files for stars not in a reference set.
 * reference set is described by an input file, named by first argument.
 * test images are additional command line arguement.s
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <math.h>
#ifdef _WIN32
#include <io.h>
#endif

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "fits.h"
#include "strops.h"
#include "wcs.h"

#define	MINSNR		3.0		/* default min SNR */
#define	MINFWHM		2.0		/* default min FWHM, pixels */
#define	MINVOL		1000.		/* default min volume, cubic pixels */

static void usage (char *p);
static void checkOneFile (FILE *fp, char *rname, char *tname);
static int findTokens (char *line, char *fields[]);
static void checkSN (FImage *rip, FImage *tip, char *rname, char *tname,
	    char *objname);
static int inaoi (double ra, double dec, double border);
static int prepOpen (char *fn, char msg[]);
static void printReport(void);

/* the individual hits are recorded and collected here as they are discovered 
 * so they can be reported all together at the end of the output.
 */
static char **report;
static int nreport;

static double hitsep;			/* allowable hit separation, rads */
static double era, ndec, wra, sdec;	/* AOI */
static double cdec;			/* cos of average AOI dec */
static double minsnr = MINSNR;		/* min snr */
static double minfwhm = MINFWHM;	/* min fwhm */
static double minvol = MINVOL;		/* min "volume" */
static int fflag;
static int vflag;

int
main (ac, av)
int ac;
char *av[];
{
	char *progname = av[0];
	char *catfile = 0;
	FILE *fp;

	while ((--ac > 0) && ((*++av)[0] == '-')) {
	    char *s;
	    for (s = av[0]+1; *s != '\0'; s++)
		switch (*s) {
		case 'f':
		    fflag++;
		    break;
		case 's':
		    if (ac < 2)
			usage (progname);
		    minsnr = atof(*++av);
		    ac--;
		    break;
		case 'o':
		    if (ac < 2)
			usage (progname);
		    minvol = atof(*++av);
		    ac--;
		    break;
		case 'v':
		    vflag++;
		    break;
		case 'w':
		    if (ac < 2)
			usage (progname);
		    minfwhm = atof(*++av);
		    ac--;
		    break;
		default:
		    usage(progname);
		}
	}

	/* now ac remaining args starting at av[0] */

	if (fflag) {
	    /* "-f file-of-files sep config" form */
	    char **fnames;
	    int nfiles;

	    if (ac != 3)
		usage (progname);

	    nfiles = readFilenames (av[0], &fnames);
	    if (nfiles == 0) {
		fprintf (stderr, "%s contains no files\n", av[0]);
		exit (1);
	    }
	    if (vflag)
		fprintf (stderr,"Found %d files in %s\n", nfiles,basenm(av[0]));
	    hitsep = degrad(atof(av[1]) / 3600.0);
	    catfile = av[2];
	    ac = nfiles;
	    av = fnames;
	} else if (ac >= 3) {
	    /* "sep config files..." form */
	    hitsep = degrad(atof(av[0]) / 3600.0);
	    catfile = av[1];
	    av += 2;
	    ac -= 2;
	} else
	    usage (progname);

	fp = fopen (catfile, "r");
	if (!fp) {
	    perror (catfile);
	    exit (1);
	}
	if (vflag)
	    fprintf (stderr, "Using %s\n", basenm(catfile));

	while (ac-- > 0) {
	    checkOneFile (fp, catfile, *av++);
	    rewind (fp);
	}

	printReport();

	return (0);
}

static void
usage (char *p)
{
	fprintf (stderr, "Usage: %s [options] min_sep config_file [test1.fts...]\n",p);
	fprintf (stderr, "%s\n", &"$Revision: 1.1.1.1 $"[1]);
	fprintf (stderr, "Purpose: produce list of stars in test images not in reference images.\n");
	fprintf (stderr, "Required arguments:\n");
	fprintf (stderr, "  min_sep     : mimumum position error to accept, arcsecs\n");
	fprintf (stderr, "  config_file : list of reference images and AOIs. Each line consists of:\n");
	fprintf (stderr, "    OBJECT EastRa NorthDec WestRa SouthDec reference.fts\n");
	fprintf (stderr, "Options:\n");
	fprintf (stderr, "  -f file : file contains list of files to test, one per line\n");
	fprintf (stderr, "  -o vol  : minumum \"volume\"; default=%g, cubic pixels\n", MINVOL);
	fprintf (stderr, "  -s snr  : minimum SNR; default=%g\n", MINSNR);
	fprintf (stderr, "  -w fwhm : minimum FWHM; default=%g, pixels\n", MINFWHM);
	fprintf (stderr, "  -v      : verbose\n");
	fprintf (stderr, "Output:\n");
	fprintf (stderr, " First 1 line for each Test file:\n");
	fprintf (stderr, "  OBJNAME RefFN TestFN DATE-OBS TIME-OBS NinRefAOI NinTestAOI NNew\n");
	fprintf (stderr, " Then NNew lines for each Test file:\n");
	fprintf (stderr, "  OBJNAME TestFN RA Dec\n");
	exit (1);
}

/* look for supernovae in tname */
static void
checkOneFile (
FILE *fp,		/* config file */
char *catfile,		/* name of catalog file */
char *tname		/* name of test file to process */
)
{
	FImage timage;		/* test image */
	char msg[1024];
	char line[256];
	char objname[128];
	int fd;
	int i;

	/* read in the test file */
	if (vflag)
	    fprintf (stderr, "Reading test %s\n", basenm(tname));
	fd = prepOpen (tname, msg);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", tname, msg);
	    return;
	}

	initFImage (&timage);
	if (readFITS (fd, &timage, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", tname, msg);
	    resetFImage (&timage);
	    close (fd);
	    return;
	}

	close (fd);

	/* pull out the object name */
	if (getStringFITS (&timage, "OBJECT", objname) < 0) {
	    fprintf (stderr, "%s: no OBJECT field\n", tname);
	    resetFImage (&timage);
	    close (fd);
	    return;
	}
	/* trim any trailing blanks */
	for (i = strlen(objname); --i >= 0; )
	    if (objname[i] == ' ')
		objname[i] = '\0';
	    else
		break;

	/* scan catalog file for valid line beginning with objname */
	while (fgets (line, sizeof(line), fp)) {
	    char *f[64];
	    int nf;

	    if (!isalnum(line[0]))
		continue;
	    nf = findTokens (line, &f[0]);
	    if (nf != 6) {
		fprintf(stderr,"Ref file line must contain 6 fields: %s",line);
		exit (1);
	    }
	    if (strcwcmp (objname, f[0]) == 0) {
		FImage rimage;	/* reference image named in file */

		/* extract the AOI from the config file line */
		if (scansex (f[1], &era) < 0) {
		    fprintf (stderr, "Bad ERA for %s: %s\n", objname, f[1]);
		    exit(1);
		}
		era = hrrad (era);
		if (scansex (f[2], &ndec) < 0) {
		    fprintf (stderr, "Bad NDec for %s: %s\n", objname, f[2]);
		    exit(1);
		}
		ndec = degrad(ndec);
		if (scansex (f[3], &wra) < 0) {
		    fprintf (stderr, "Bad WRA for %s: %s\n", objname, f[3]);
		    exit(1);
		}
		wra = hrrad (wra);
		if (scansex (f[4], &sdec) < 0) {
		    fprintf (stderr, "Bad SDec for %s: %s\n", objname, f[4]);
		    exit(1);
		}
		sdec = degrad(sdec);
		cdec = cos ((ndec + sdec)/2);

		/* open the reference image named in the config file */
		if (vflag)
		    fprintf (stderr, "Reading reference %s\n", basenm(f[5]));
		fd = prepOpen (f[5], msg);
		if (fd < 0) {
		    fprintf (stderr, "%s: %s\n", f[5], msg);
		    continue;
		}
		initFImage (&rimage);
		if (readFITS (fd, &rimage, msg) < 0) {
		    fprintf (stderr, "%s: %s\n", f[5], msg);
		    exit(1);
		}
		close (fd);

		/* check it */
		checkSN (&rimage, &timage, f[5], tname, objname);

		/* all done with the test and reference images */
		resetFImage (&timage);
		resetFImage (&rimage);

		return;
	    }
	}

	fprintf (stderr, "No reference file listed for object %s\n", objname);
	resetFImage (&timage);
}

/* break line at each white-space and fill (*f)[i] with pointer to each token.
 * we plug '\0' into line IN-PLACE at the end of each token.
 * return number of tokens found.
 */
static int
findTokens (char line[], char **f)
{
	typedef enum {INTOK, INWHITE} State;
	char c;
	State state;
	int n;

	/* assume one */
	f[0] = line;
	n = 1;
	state = INTOK;

	for (; (c = *line) != '\0'; line++) {
	    switch (state) {
	    case INTOK:
		if (isspace(c)) {
		    *line = '\0';
		    state = INWHITE;
		}
		break;
	    case INWHITE:
		if (!isspace(c)) {
		    *++f = line;
		    state = INTOK;
		    n++;
		}
		break;
	    }
	}

	return (n);
}

/* return 0 if stats qualify, else -1 */
static int
qualify (StarStats *ssp)
{
	if (ssp->xfwhm*ssp->yfwhm*(ssp->p - ssp->Sky) < minvol) return (-1);
	if ((ssp->p - ssp->Sky)/ssp->rmsSky < minsnr) return (-1);
	if (ssp->xfwhm > 2*ssp->yfwhm) return (-1);
	if (ssp->xfwhm < ssp->yfwhm/2) return (-1);
	if ((ssp->xfwhm+ssp->yfwhm)/2 < minfwhm) return (-1);
	return (0);
}

/* look through tip for entries not in rip */
static void
checkSN (
FImage *rip,	/* reference image */
FImage *tip,	/* candidate (new/test) image */
char *rname,	/* name of reference file */
char *tname,	/* name of test file */
char *objname	/* name of object */
)
{
	static char huh[] = "????????";
	int *rx, *ry;			/* ref x,y locations */
	double *rr, *rd;		/* ref ra/dec locations */
	CamPixel *rb;			/* reference star brightnesses */
	int nr, nraoi;			/* num ref stars, and num within aoi */
	int *tx, *ty;			/* test x,y locations */
	double *tr, *td;		/* test ra/dec locations */
	CamPixel *tb;			/* test star brightnesses */
	int nt, ntaoi;			/* num test stars, and num within aoi */
	int nh;				/* total number of hits */
	int ns;				/* n stars before scrub */
	FITSRow dateobs, timeobs;
	StarDfn sd;
	int r, t;


	/* set up a default aperture */
	sd.rAp = 0;
	sd.rsrch = 10;
	sd.how = SSHOW_HERE;

	/* find qualifying reference stars and convert to ra/dec.
	 * also get a count of how many are really inside the aoi.
	 */
	ns = findStars (rip->image, rip->sw, rip->sh, &rx, &ry, &rb);
	rr = (double *) malloc (ns * sizeof(double));
	rd = (double *) malloc (ns * sizeof(double));
	nraoi = 0;
	nr = 0;
	for (r = 0; r < ns; r++) {
	    StarStats ss;
	    char buf[1024];

	    if (starStats ((CamPixel *)rip->image, rip->sw, rip->sh, &sd,
						    rx[r], ry[r], &ss, buf) < 0
			|| qualify (&ss) < 0)
		continue;
	    xy2RADec (rip, ss.x, ss.y, &rr[nr], &rd[nr]);
	    if (inaoi (rr[nr], rd[nr], 0.0) == 0)
		nraoi++;
	    nr++;
	}
	if (vflag)
	    fprintf (stderr, "%d of %d stars qualify in reference %s\n", nr, ns, basenm(rname));


	/* find qualifying test stars and convert to ra/dec.
	 * also get a count of how many are really inside the aoi.
	 */
	ns = findStars (tip->image, tip->sw, tip->sh, &tx, &ty, &tb);
	tr = (double *) malloc (ns * sizeof(double));
	td = (double *) malloc (ns * sizeof(double));
	ntaoi = 0;
	nt = 0;
	for (t = 0; t < ns; t++) {
	    StarStats ss;
	    char buf[1024];

	    if (starStats ((CamPixel *)tip->image, tip->sw, tip->sh, &sd,
						    tx[t], ty[t], &ss, buf) < 0
			|| qualify (&ss) < 0)
		continue;
	    xy2RADec (tip, ss.x, ss.y, &tr[nt], &td[nt]);
	    if (inaoi (tr[nt], td[nt], 0.0) == 0)
		ntaoi++;
	    nt++;
	}
	if (vflag)
	    fprintf (stderr, "%d of %d stars qualify in test %s\n", nt, ns, basenm(tname));

	/* search each test star for one not in the ref image.
	 * all must be within the aoi.
	 * TODO: sort the lists for faster checking.
	 */
	nh = 0;
	for (t = 0; t < nt; t++) {
	    double sintd, costd;
	    int found;

	    if (inaoi (tr[t], td[t], hitsep/cdec) < 0)
		continue;

	    sintd = sin(td[t]);
	    costd = cos(td[t]);

	    /* look for a ref star that lies pretty close to this test star */
	    found = 0;
	    for (r = 0; r < nr; r++) {
		double csep, tmp;

		if (inaoi (rr[r], rd[r], 0.0) < 0)
		    continue;

		solve_sphere (tr[t]-rr[r], PI/2-rd[r], sintd, costd,&csep,&tmp);

		if (acos(csep) < hitsep) {
		    found = 1;
		    break;	/* r[r] matches t[t] well enough */
		}
	    }

	    /* if none found, we found a candidate */
	    if (!found) {
		/* add to detailed report */
		char rstr[64], dstr[64];
		char rpt[256];

		fs_sexa (rstr, radhr(tr[t]), 2, 360000);
		fs_sexa (dstr, raddeg(td[t]), 3, 36000);
		sprintf (rpt, "%-12s %-12s %s %s", objname, basenm(tname),
								    rstr, dstr);

		report = (char **) realloc (report, (nreport+1)*sizeof(char *));
		report[nreport] = malloc (strlen(rpt)+1);
		strcpy (report[nreport], rpt);
		nreport++;

		nh++;
	    }
	}

	/* print summary report now */
	if (getStringFITS (tip, "DATE-OBS", dateobs) < 0) {
	    fprintf (stderr, "No DATE-OBS field in test image\n");
	    strcpy (dateobs, huh);
	}
	if (getStringFITS (tip, "TIME-OBS", timeobs) < 0) {
	    fprintf (stderr, "No TIME-OBS field in test image\n");
	    strcpy (timeobs, huh);
	}
	printf ("%-12s %-12s %-12s %s %s %3d %3d %3d\n", objname,
	    basenm(rname), basenm(tname), dateobs, timeobs, nraoi,ntaoi,nh);


	free ((char *)tx);
	free ((char *)ty);
	free ((char *)tr);
	free ((char *)td);
	free ((char *)tb);

	free ((char *)rx);
	free ((char *)ry);
	free ((char *)rr);
	free ((char *)rd);
	free ((char *)rb);
}

/* return 0 if the given ra/dec is inside box bounded by era/ndec wra/sdec.
 * if not return -1.
 * b is an additional border that must be met within the aoi.
 * TODO: wrap at 24 HR ra.
 */
static int
inaoi (r, d, b)
double r, d;	/* ra, dec, rads */
double b;	/* border, rads */
{
	return (r > (era-b) || r < (wra+b) || d > (ndec-b) || d < (sdec+b)
			? -1 : 0);
}

/* open fn for reading and return the fd.
 * if fn ends in .fth we copy to a tmp location and apply fdecompress.
 * if all is well, return fd, else fill errmsg[] and return -1.
 */
static int
prepOpen (fn, errmsg)
char fn[];
char errmsg[];
{
	int fd;
	int l;

	l = strlen (fn);
	if (l < 4 || (strcmp(fn+l-4, ".fth") && strcmp(fn+l-4, ".FTH"))) {
	    /* just open directly */
#ifdef _WIN32
	    fd = open (fn, O_RDONLY | O_BINARY);
#else
	    fd = open (fn, O_RDONLY);
#endif
	    if (fd < 0)
		strcpy (errmsg, strerror(errno));
	} else {
	    /* ends with .fth so need to run through fdecompress
	     * TODO: use White's new inline code when it comes out.
	     */
	    char cmd[2048];
	    char *tmpp, tmp[1024];
	    int s, tl;
	    int mode;

	    tmpp = tmpnam (NULL);
	    if (!tmpp) {
		sprintf (errmsg, "Can not create temp filename for fdecomp");
		return (-1);
	    }
	    sprintf (tmp, "%s.fth", tmpp);
	    tl = strlen(tmp);

#ifdef _WIN32
	    /* use copy and do it in two steps */
	    sprintf (cmd, "copy %s %s", fn, tmp);
	    s = system (cmd);
	    if (s != 0) {
		sprintf (errmsg, "Can not execute `%s' ", cmd);
		return (-1);
	    }
	    sprintf (cmd, "fdecomp -r %s", tmp);
	    s = system (cmd);
	    if (s != 0) {
		sprintf (errmsg, "Can not execute `%s' ", cmd);
		(void) remove (tmp);	/* remove the .fth copy */
		return (-1);
	    }

	    mode = O_RDONLY | O_BINARY;
#else
	    sprintf (cmd, "cp %s %s; fdecompress -r %s", fn, tmp, tmp);
	    s = system (cmd);
	    if (s != 0) {
		sprintf (errmsg, "Can not execute `%s' ", cmd);
		(void) remove (tmp);	/* remove the .fth copy */
		return (-1);
	    }

	    mode = O_RDONLY;
#endif

	    (void) remove (tmp);	/* remove the .fth copy */

	    /* open the uncompressed file */
	    tmp[tl-1] = tmp[tl-1] == 'h' ? 's' : 'S';
	    fd = open (tmp, mode);
	    (void) unlink (tmp);	/* remove the .fts copy */
	    if (fd < 0)
		sprintf (errmsg, "Can not decompress %s", fn);
	}

	return (fd);
}

static void
printReport()
{
	int i;

	printf ("\f\n");
	for (i = 0; i < nreport; i++) {
	    if (i > 0 && strncasecmp (report[i-1], report[i], 12))
		putchar ('\n');
	    printf ("%s\n", report[i]);
	}
}
