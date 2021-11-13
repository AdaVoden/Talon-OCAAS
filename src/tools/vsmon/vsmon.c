/* scan a directory for all images found in a catalog and report magnitudes.
 * used to monitor variable stars.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telenv.h"
#include "fits.h"
#include "photstd.h"
#include "wcs.h"

extern void heliocorr (double jd, double ra, double dec, double *hcp);

#define	DEF_RMAX	12		/* default radius to hunt for star */
#define	DEF_SSHOW	SSHOW_MAXINAREA	/* default search method */

char *catdir_def = "archive/photcal";
char *imdir_def   = "user/images";

char *catfile_def = "vsmon.lst";
char *outfile_def = "vsmon.out";

char *catfile;
char *outfile;
char *imdir;

int verbose;

/* filter indeces */
enum {BF, VF, RF, IF, NF};

/* info from a line from the catalog file */
typedef struct {
    char *name;		/* malloced object name */
    double tra, tdec;	/* target star loc, rads */
    double tm[NF];	/* target star true trigger BVRI mags */
    double cra, cdec;	/* calibrator star loc, rads */
    double cm[NF];	/* calibrator star true BVRI mags */
    double kra, kdec;	/* check star loc, rads */
    double km[NF];	/* check star true BVRI mags */
} Cat;

Cat *cat;	/* malloced list of catalog entries */
int ncat;	/* number of items in list */

SSHow sshow = DEF_SSHOW;
int rmax = DEF_RMAX;

static void usage (char *p);
static char *dirfn (char *dir, char *fn);
static void readCatFile (char *fn);
static void addCat (char *name, Cat *cp);
static void vsmon (char *imdir, FILE *outfp);
static int fitsFilename (char path[]);
static void processFile (char fullfn[], char fn[], FILE *outfp);
static void processStars (char fn[], FImage *fip, double jd, double Z,
    int filter, double dur, Cat *cp, FILE *outfp);
static int findSS (FImage *fip, double ra, double dur, double dec,
    StarStats *sp, char msg[]);
static void report (char *fn, double jd, double Z, int filter, Cat *cp,
    double T, double Te, double K, double Ce, FILE *fp);

int
main (int ac, char *av[])
{
	char *progname = av[0];
	FILE *outfp;
	char *str;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    /* in the loop ac includes the current arg */
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'b':	/* do brightwalk search method instead */
		    sshow = SSHOW_BRIGHTWALK;
		    break;
		case 'c':	/* alternate catalog file name */
		    if (ac < 2) {
			fprintf (stderr, "-c requires catalog file name\n");
			usage (progname);
		    }
		    catfile = *++av;
		    ac--;
		    break;
		case 'i':	/* alternate image directory */
		    if (ac < 2) {
			fprintf (stderr, "-d requires image directory\n");
			usage (progname);
		    }
		    imdir = *++av;
		    ac--;
		    break;
		case 'o':	/* alternate output file name */
		    if (ac < 2) {
			fprintf (stderr, "-o requires output file name\n");
			usage (progname);
		    }
		    outfile = *++av;
		    ac--;
		    break;
		case 'r':	/* specify star search radius */
		    if (ac < 2) {
			fprintf (stderr, "-r requires search radius\n");
			usage (progname);
		    }
		    rmax = atoi(*++av);
		    ac--;
		    break;
		case 'v':	/* trace file activity */
		    verbose++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0] */
	if (ac > 0) {
	    fprintf (stderr, "Extra arguments\n");
	    usage (progname);
	}

	if (verbose) {
	    fprintf (stderr, "Rmax = %d\n", rmax);
	    fprintf (stderr, "Search method = %s\n",
		sshow == SSHOW_BRIGHTWALK ? "BrightWalk" : "MaxInArea");
	}

	/* form final names */
	if (!catfile)
	    catfile = dirfn (catdir_def, catfile_def);
	if (!outfile)
	    outfile = dirfn (catdir_def, outfile_def);
	if (!imdir)
	    imdir = dirfn (imdir_def, NULL);

	/* open and read in the catalog file */
	readCatFile(catfile);

	/* open the output file -- beware of "-" for stdout */
	if (strcmp (outfile, "-") == 0)
	    outfp = stdout;
	else {
	    outfp = fopen (outfile, "a");
	    if (!outfp) {
		fprintf (stderr, "%s: %s\n", outfile, strerror(errno));
		exit(1);
	    }
	}

	/* go */
	vsmon(imdir, outfp);

	return (0);
}

static void
usage (char *p)
{
	FILE *fp = stderr;

	fprintf(fp,"usage: %s [options]\n", p);
	fprintf(fp,"  -b     : use bright-walk star search method. default is max-in-area.\n");
	fprintf(fp,"  -c file: alternate catalog file.\n");
	fprintf(fp,"           default is %s/%s\n", catdir_def, catfile_def);
	fprintf(fp,"  -i file: alternate image directory.\n");
	fprintf(fp,"           default is %s\n", imdir_def);
	fprintf(fp,"  -o file: alternate output file; use - for stdout.\n");
	fprintf(fp,"           default is %s/%s\n", catdir_def, outfile_def);
	fprintf(fp,"  -r rmax: max radius to search for star, pixels. default is %d\n", DEF_RMAX);
	fprintf(fp,"  -v:      verbose\n");
	exit(1);
}

/* if not NULL, append filename fn to directory dir into a fresh malloc copy.
 * either way, also handle TELHOME too.
 */
static char *
dirfn (char *dir, char *fn)
{
	char buf[1024];
	char *mem;

	if (fn)
	    sprintf (buf, "%s/%s", dir, fn);
	else
	    strcpy (buf, dir);
	telfixpath (buf, buf);

	mem = malloc (strlen(buf)+1);
	if (mem == NULL) {
	    fprintf (stderr, "No mem\n");
	    exit (1);
	}
	strcpy (mem, buf);

	return (mem);
}

static void
readCatFile (char *fn)
{
	FILE *fp;
	char buf[1024];
	Cat c;
	int n;

	/* open the catalog file */
	fp = fopen (fn, "r");
	if (!fp) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    exit(1);
	}

	/* read all of it in groups of 3 lines: C/K/T */
	n = 0;
	while (fgets (buf, sizeof(buf), fp)) {
	    char name[64];
	    char rstr[64], dstr[64];
	    char code[64];
	    double b, v, r, i;
	    double bv, vr, vi;
	    double ra, dec;
	    int ns;

	    /* skip comment and blank lines */
	    if (buf[0] == '#' || buf[0] == '\n')
		continue;

	    /* crack a line */
	    v = bv = vr = vi = 0;
	    ns = sscanf (buf, "%s %s %s %s %lf %lf %lf %lf", code, name, rstr,
						    dstr, &v, &bv, &vr, &vi);
	    if (ns < 5) {
		fprintf (stderr, "%s: Bad line: %s", fn, buf);
		exit(1);
	    }

	    /* convert loc strings to rads */
	    if (scansex (rstr, &ra) < 0) {
		fprintf (stderr, "%s: bad RA: %s", fn, buf);
		exit(1);
	    }
	    if (scansex (dstr, &dec) < 0) {
		fprintf (stderr, "%s: bad Dec: %s", fn, buf);
		exit(1);
	    }
	    ra = hrrad(ra);
	    dec = degrad(dec);

	    /* compute the derived colors */
	    b = bv+v;
	    r = v-vr;
	    i = v-vi;

	    /* dispense line depending on code */
	    switch (code[0]) {
	    case 'c': case 'C':
		if ((n%3)!=0) {
		    fprintf (stderr, "%s: CKT sequence error near: %s", fn,buf);
		    exit(1);
		}
		c.cm[BF] = b;
		c.cm[VF] = v;
		c.cm[RF] = r;
		c.cm[IF] = i;
		c.cra = ra;
		c.cdec = dec;
		break;
	    case 'k': case 'K':
		if ((n%3)!=1) {
		    fprintf (stderr, "%s: CKT sequence error near: %s", fn,buf);
		    exit(1);
		}
		c.km[BF] = b;
		c.km[VF] = v;
		c.km[RF] = r;
		c.km[IF] = i;
		c.kra = ra;
		c.kdec = dec;
		break;
	    case 't': case 'T':
		if ((n%3)!=2) {
		    fprintf (stderr, "%s: CKT sequence error near: %s", fn,buf);
		    exit(1);
		}
		c.tm[BF] = b;
		c.tm[VF] = v;
		c.tm[RF] = r;
		c.tm[IF] = i;
		c.tra = ra;
		c.tdec = dec;
		break;
	    default:
		fprintf (stderr, "%s: unknown star code: %s", fn, buf);
		exit(1);
	    }

	    /* add to list every third line */
	    if ((n%3) == 2)
		addCat (name, &c);
	    n++;
	}

	/* that's it */
	fclose (fp);
}

static void
addCat (char *name, Cat *cp)
{
	char *mem;

	mem = cat ? realloc ((void *)cat, (ncat+1)*sizeof(Cat))
		  : malloc (sizeof(Cat));
	if (!mem) {
	    fprintf (stderr, "No catalog mem\n");
	    exit (1);
	}
	cat = (Cat *)mem;

	cp->name = strcpy (malloc (strlen (name)+1), name);
	if (!cp->name) {
	    fprintf (stderr, "No name mem\n");
	    exit (1);
	}

	cat[ncat++] = *cp;
}

/* scan the given dir for images in cat[] and print results to outfp.
 */
static void
vsmon (char *dir, FILE *outfp)
{
	DIR *dirp;
	struct dirent *dp;

	dirp = opendir (dir);
	if (!dirp) {
	    fprintf (stderr, "%s: %s\n", dir, strerror(errno));
	    return;
	}

	if (verbose)
	    fprintf (stderr, "Scanning for images in \"%s\"\n", dir);
	while ((dp = readdir (dirp)) != NULL) {
	    if (fitsFilename (dp->d_name) == 0) {
		char full[2048];
		sprintf (full, "%s/%s", dir, dp->d_name);
		processFile (full, dp->d_name, outfp);
	    }
	}

	(void) closedir (dirp);
}

/* return 0 if the given filename ends with .fts, else -1. */
static int
fitsFilename (char path[])
{
	int l = strlen (path);
	char *suf = path+l-4;

	return (l>4 && (!strcmp (suf, ".fts") || !strcmp(suf,".FTS")) ? 0 : -1);
}

static void
processFile (char fullfn[], char fn[], FILE *outfp)
{
	char msg[1024];
	FImage fim, *fip = &fim;
	FITSRow row;
	Cat *cp;
	double jd;
	double el, Z;
	double dur;
	int filter;
	int fd;
	int i;

	if (verbose)
	    fprintf (stderr, "Reading %s\n", fn);

	/* open the FITS file */
	fd = open (fullfn, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    return;
	}
	if (readFITSHeader (fd, fip, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, msg);
	    close (fd);
	    return;
	}

	/* confirm image has all WCS fields */
	if (checkWCSFITS (fip, 0) < 0) {
	    fprintf (stderr, "%s: no WCS fields\n", fn);
	    goto done;
	}

	/* get the object name and set cp if it is in the Cat list */
	if (getStringFITS (fip, "OBJECT", row) < 0) {
	    fprintf (stderr, "%s: No OBJECT field\n", fn);
	    goto done;
	}
	for (i = 0; i < ncat; i++)
	    if (strcasecmp (cat[i].name, row) == 0)
		break;
	if (i == ncat) {
	    fprintf (stderr, "%s: OBJECT %s not in catalog\n", fn, row);
	    goto done;
	}
	cp = &cat[i];

	/* get elevation and compute airmass */
	if (getStringFITS (fip, "ELEVATION", row) < 0) {
	    fprintf (stderr, "%s: no ELEVATION field\n", fn); 
	    goto done;
	}
	if (scansex (row, &el) < 0) {
	    fprintf (stderr, "%s: bad ELEVATION field: %s\n", fn, row); 
	    goto done;
	}
	Z = 1.0/sin(degrad(el));

	/* get filter */
	if (getStringFITS (fip, "FILTER", row) < 0) {
	    fprintf (stderr, "%s: no FILTER field.\n", fn); 
	    goto done;
	}
	switch (row[0]) {
	case 'b': case 'B': filter = BF; break;
	case 'v': case 'V': filter = VF; break;
	case 'r': case 'R': filter = RF; break;
	case 'i': case 'I': filter = IF; break;
	default: goto done;	/* just skip other filters */
	}

	/* get exposure time */
	if (getRealFITS (fip, "EXPTIME", &dur) < 0) {
	    fprintf (stderr, "%s: no EXPTIME field.\n", fn); 
	    goto done;
	}

	/* get JD time */
	if (getRealFITS (fip, "JD", &jd) < 0) {
	    fprintf (stderr, "%s: no JD field.\n", fn); 
	    goto done;
	}

	/* Phew! now it's worth reading the pixels and dig out the star stats */
	resetFImage (fip);
	lseek (fd, 0, 0);
	if (readFITS (fd, fip, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, msg);
	    goto done;
	}

	processStars (fn, fip, jd, Z, filter, dur, cp, outfp);

	/* done all we can */
    done:
	resetFImage (fip);
	close (fd);
}

static void
processStars (char fn[], FImage *fip, double jd, double Z, int filter,
double dur, Cat *cp, FILE *outfp)
{
	char msg[1024];
	char rstr[32], dstr[32];
	StarStats st, sc, sk;
	double T, Te, K, Ke;
	double m;

	/* find target star stats */
	if (findSS (fip, cp->tra, cp->tdec, dur, &st, msg) < 0) {
	    fs_sexa (rstr, radhr(cp->tra), 2, 3600);
	    fs_sexa (dstr, raddeg(cp->tdec), 4, 3600);
	    fprintf (stderr, "%s: target star: %s %s %s\n", fn, rstr, dstr,msg);
	    return;
	}

	/* find calibrator star stats */
	if (findSS (fip, cp->cra, cp->cdec, dur, &sc, msg) < 0) {
	    fs_sexa (rstr, radhr(cp->cra), 2, 3600);
	    fs_sexa (dstr, raddeg(cp->cdec), 4, 3600);
	    fprintf (stderr,"%s: calibrator star: %s %s %s\n",fn,rstr,dstr,msg);
	    return;
	}

	/* find check star stats */
	if (findSS (fip, cp->kra, cp->kdec, dur, &sk, msg) < 0) {
	    fs_sexa (rstr, radhr(cp->kra), 2, 3600);
	    fs_sexa (dstr, raddeg(cp->kdec), 4, 3600);
	    fprintf (stderr, "%s: check star: %s %s %s\n", fn, rstr, dstr,msg);
	    return;
	}

	(void) starMag (&sc, &st, &m, &Te);	/* m = mag(t) - mag(c) */
	T = cp->cm[filter] + m;			/* abs mag of target */
	(void) starMag (&sc, &sk, &m, &Ke);	/* m = mag(k) - mag(c) */
	K = cp->cm[filter] + m;			/* abs mag of check */

	report (fn, jd, Z, filter, cp, T, Te, K, Ke, outfp);
}

/* find the stats of the star at the given location.
 * return 0 if ok, else fill msg[] and return -1.
 */
static int
findSS (FImage *fip, double ra, double dec, double dur,StarStats *sp,char msg[])
{
	double x, y;
	StarDfn sd;

	/* prepare for starStats */
	sd.rsrch = rmax;
	sd.rAp = ABSAP;
	sd.how = sshow;

	/* find star in x/y */
	if (RADec2xy (fip, ra, dec, &x, &y) < 0) {
	    sprintf (msg, "Can not convert RA/Dec (%g/%g, rads) to X/Y\n",
								    ra, dec);
	    return (-1);
	}

	/* find stats */
	if (starStats ((CamPixel *)fip->image, fip->sw, fip->sh, &sd, (int)x,
							(int)y, sp, msg) < 0)
	    return (-1);

	return (0);
}

static void
report (char *fn, double jd, double Z, int filter, Cat *cp, double T, double Te,
double K, double Ke, FILE *fp)
{
	double Mjd;
	double hc;
	char str[64];

	/* find heliocentric time */
	heliocorr (jd, cp->tra, cp->tdec, &hc);
	jd -= hc;	/* geocentric to heliocentric */
	Mjd = jd - MJD0;

	/* file name */
	fprintf (fp, "%12s", fn);

	/* object name */
	fprintf (fp, " %10s", cp->name);

	/* date */
	fs_date (str, mjd_day(Mjd));
	fprintf (fp, " %s", str);

	/* time */
	fs_sexa (str, mjd_hr(Mjd), 2, 3600);
	fprintf (fp, " %s", str);

	/* JD */
	fprintf (fp, " %13.5f", jd);

	/* airmass */
	fprintf (fp, " %6.3f", Z);

	/* filter */
	switch (filter) {
	case BF: fprintf (fp, " B"); break;
	case VF: fprintf (fp, " V"); break;
	case RF: fprintf (fp, " R"); break;
	case IF: fprintf (fp, " I"); break;
	default: fprintf (fp, " ?"); break;
	}

	/* observed target mag and error */
	fprintf (fp, " %6.3f %5.3f", T, Te);

	/* target mag flag */
	fprintf (fp, " %c", T <= cp->tm[filter] ? 'Y' : 'N');

	/* deviation from expected check star magnitude */
	fprintf (fp, " %6.3f", K - cp->km[filter]);

	/* check mag error */
	fprintf (fp, " %5.3f", Ke);

	/* that's it */
	fprintf (fp, "\n");
}
