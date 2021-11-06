/* given BVRI images for the same field taken within a short time span and a
 * set of photometric calibration constants either from photcal.out or .def,
 * extract each star common to all images and write out their BVRI values and
 * errors, one per line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "strops.h"
#include "telenv.h"
#include "fits.h"
#include "photstd.h"
#include "wcs.h"

#define	PCDT	12.0	/* max time span to look for photo constants, hours */
#define	IDT	1.0	/* max time interval between images, hours */
#define	STARSZ	5.0	/* star location radius, arc seconds */
#define	RB	4	/* max radius to search for brightest pixel */
#define	RMAX	15	/* max radius from brightest pixel to star edge */
#define	CHG	0.01	/* star ends when pixel sums in expanding disks don't
			 * increase by at least this proportion
			 */
/* default directories and file pathnames */
static char calfn_def[] = "photcal.out";
static char *calfn = calfn_def;
static char deffn_def[] = "photcal.def";
static char *deffn = deffn_def;
static char auxdir_def[] = "archive/photcal";
static char *auxdir = auxdir_def;

/* net pathnames */
static char *calpath;
static char *defpath;

static int verbose;	/* >0 gives more info on stderr */

/* indices into various data structures for the 4 filters */
enum {BF, VF, RF, IF, NF};

/* what we know from each image */
typedef struct {
    int ok;		/* set when we know this stuff */
    char fname[32];	/* base of filename */
    double exptime;	/* secs */
    double Z;     	/* airmass = 1/sin(alt) */
    StarStats *ssp;	/* malloced list of StarStats */
    double *rap, *decp;	/* malloced list of corresponding star positions */
    double jd;		/* time of exposure, JD */
    int nstars;		/* number of entries in ssp, rap and decp */
} IParams;

static IParams iparams[NF];

static char filtnames[NF] = "BVRI";	/* handy filter enum to letter table */

/* calibration info */
typedef struct {
    double V0;		/* instr correction */
    double V0e;		/* instr correction error */
    double kp;		/* kp value */
    double kpe;		/* kp error */
    double kpp;		/* kpp value */
    double kppe;	/* kpp error */
} CalConst;

static CalConst calconst[NF];

/* this is a list of indeces into each of the iparam star info fields such that
 * they will all refer to the same star. we build it by scanning the B image
 * stars and finding stars which also exist in each of the VRI images.
 *
 * for example:
 *   iparam[BF].ssp[starsets[i][BF]]
 * is the same star as
 *   iparam[VF].ssp[starsets[i][VF]]
 */
typedef int StarSet[NF];
static StarSet *starsets;
static int nstarsets;

static StarDfn stardef;

static void usage (char *prog);
static char *dirfn (char *dir, char *fn);
static void readConfigFile(char *fn);
static char *oneGoodLine (char *buf, int n, FILE *fp);
static void readImage (char *filename);
static void readImageFiles (char *cfn, FILE *fp, char *initbuf);
static int getFilter (FImage *fip, int *codep, char *msg);
static int getAirmass (FImage *fip, double *amp, char *msg);
static int getExptime (FImage *fip, double *expp, char *msg);
static int getJD (FImage *fip, double *jdp, char *msg);
static void check4Filters(void);
static double photJD (void);
static void readCalConst(double jd);
static void fillCalConst (PCalConst *pccp);
static void findCommonStars(void);
static int sameStar (double ra0, double dec0, double ra1, double ra2);
static double normalizedVobs (StarStats *sp, double exp);
static void doReport();
static void findMags (int i, double V[NF], double err[NF]);
static void findStarMag (int star, int fcode,double bv,double *Vp,double *errp);
static void printStar (int i, double V[], double err[]);

int
main (int ac, char *av[])
{
	char *progname = av[0];
	char *str;
	double jd;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'c':	/* alternate calibration file */
		    if (ac < 2)
			usage (progname);
		    calfn = *++av;
		    ac--;
		    break;
		case 'd':	/* alternate default calibration file */
		    if (ac < 2)
			usage (progname);
		    deffn = *++av;
		    ac--;
		    break;
		case 'p':	/* alternate dir */
		    if (ac < 2)
			usage (progname);
		    auxdir = *++av;
		    ac--;
		    break;
		case 'v':	/* more verbosity */
		    verbose++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0].
	 * it should be the name of a config file.
	 */
	if (ac != 1)
	    usage (progname);

	/* set up net path names to things */
	calpath = dirfn (auxdir, calfn);
	defpath = dirfn (auxdir, deffn);

	/* read the star definition and images from the config file */
	readConfigFile(av[0]);

	/* make sure we found images for all four filters */
	check4Filters();

	/* establish the effective photometric date */
	jd = photJD();

	/* look around for photometric constants and fill calconst[] */
	readCalConst(jd);

	/* find the set of stars in common to all images */
	findCommonStars();

	/* finally! */
	if (verbose)
	    printf ("Found %d star sets:\n", nstarsets);
	doReport();

	return (0);
}

static void
usage (char *prog)
{
    FILE *fp = stderr;

    fprintf(fp,"%s: [options] config_file\n", prog);
    fprintf(fp,"Options:\n");
    fprintf(fp,"  -c file: photmetric constants file.\n");
    fprintf(fp,"           default is %s\n", calfn_def);
    fprintf(fp,"  -d file: default file to use when date not found.\n");
    fprintf(fp,"           default is %s.\n", deffn_def);
    fprintf(fp,"  -p file: directory of out/def files;\n");
    fprintf(fp,"           default is %s\n", auxdir_def);
    fprintf(fp,"  -v:      verbose.\n");
    fprintf(fp,"Required:\n");
    fprintf(fp,"  config_file\n");

    exit (1);
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


/* define stardef */
static void
readConfigFile(char *fn)
{
	FILE *fp;
	char buf[1024];
	int n;
	double tmp;

	/* open the file */
	fp = fopen (fn, "r");
	if (!fp) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    exit (1);
	}

	/* first good line is rsrch and rap */
	if (oneGoodLine (buf, sizeof(buf), fp) == NULL) {
	    fprintf (stderr, "%s: file is short\n", fn);
	    exit(1);
	}

	n = sscanf(buf, "%d %d %lf", &stardef.rsrch, &stardef.rAp, &tmp);
	if (n == 3) {
	    fprintf (stderr, "%s: appears to be old format config file\n", fn);
	    fprintf (stderr, "%s: expecting RSrch Raperture\n", fn);
	    exit(1);
	}
	if (n != 2) {
	    fprintf (stderr, "%s: expecting RSrch Raperture\n", fn);
	    exit(1);
	}
	stardef.how = SSHOW_HERE;

	if (verbose)
	    printf ("Star Defn: rsrch=%d rAp=%d\n", stardef.rsrch,
								stardef.rAp);

	/* next good line might be name of a file with 4 filenames or they
	 * might be right here.
	 */
	if (oneGoodLine (buf, sizeof(buf), fp) == NULL) {
	    fprintf (stderr, "%s: Filename or names expected\n", fn);
	    exit(1);
	}
	buf[strlen(buf)-1] = '\0';
	if (buf[0] == '*') {
	    FILE *indirfp;

	    if (strlen(buf) < 2) {
		fprintf (stderr, "%s: Bad file indirect: %s\n", fn, buf);
		exit (1);
	    }

	    indirfp = fopen (buf+1, "r");
	    if (!indirfp) {
		fprintf (stderr, "%s: can not open %s\n", fn, buf+1);
		exit (1);
	    }

	    readImageFiles (fn, indirfp, NULL);
	    fclose (indirfp);
	} else
	    readImageFiles (fn, fp, buf);

	(void) fclose (fp);
}

/* read lines from fp until find one not blank and not starting with #.
 * if ok return buf, else NULL (just like fgets()
 */
static char *
oneGoodLine (char *buf, int n, FILE *fp)
{
	while (fgets (buf, n, fp))
	    if (!(buf[0] == '#' || buf[0] == '\n'))
		return (buf);
	return (NULL);
}

/* read in four filenames of BVRI images from fp.
 * if initbuf!=NULL then its one line and we just read 3 more from fp.
 * cfn is the name of the config file (used just for error messages).
 * exit if trouble.
 */
static void
readImageFiles (char *cfn, FILE *fp, char *initbuf)
{
	char buf[1024];
	int i, n;

	if (initbuf) {
	    readImage (initbuf);
	    n = 3;
	} else
	    n = 4;

	for (i = 0; i < n; i++) {
	    if (oneGoodLine (buf, sizeof(buf), fp) == NULL) {
		fprintf (stderr, "%s: only found %d of 4 filenames.\n", cfn, i);
		exit(1);
	    }
	    buf[strlen(buf)-1] = '\0';

	    /* read the image, dig out the stars and fill an iparams[] */
	    readImage (buf);
	}
}

/* read the given FITS file, and load up the corresponding iparams entry. no
 * filter dups. exit on error.
 */
static void
readImage (char *filename)
{
	char msg[1024];
	char *base;
	int fenum;
	int fd;
	FImage fim;
	IParams *ip;
	int *xa, *ya;
	CamPixel *ba;
	int nstars, ngood;
	int w, h;
	int i;

	/* open and read the image */
	base = basenm(filename);
	fd = open (filename, O_RDONLY);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", filename, strerror(errno));
	    exit (1);
	}
	if (readFITS (fd, &fim, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", base, msg);
	    exit (1);
	}

	/* see which filter it is */
	if (getFilter (&fim, &fenum, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", base, msg);
	    exit(1);
	}

	/* establish which iparams to use -- allow no dups */
	ip = &iparams[fenum];
	if (iparams[fenum].ok) {
	    fprintf (stderr, "Both %s and %s are filter %c\n", ip->fname, base,
							    filtnames[fenum]);
	    exit (1);
	}
	strncpy (ip->fname, base, sizeof(ip->fname)-1);

	/* get airmass, exposure time and JD */
	if (getAirmass (&fim, &ip->Z, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", base, msg);
	    exit(1);
	}
	if (getExptime (&fim, &ip->exptime, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", base, msg);
	    exit(1);
	}
	if (getJD (&fim, &ip->jd, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", base, msg);
	    exit (1);
	}

	/* easy access to image size */
	w = fim.sw;
	h = fim.sh;

	/* find the stars */
	nstars = findStars (fim.image, w, h, &xa, &ya, &ba);
	if (nstars <= 0) {
	    fprintf (stderr, "%s: no stars\n", base);
	    exit (1);
	}

	if (verbose) {
	    char dstr[128];
	    char tstr[128];

	    getStringFITS (&fim, "DATE-OBS", dstr);
	    getStringFITS (&fim, "TIME-OBS", tstr);
	    printf ("%s: %s %s %gS %c Z=%.2f %d stars\n", base, dstr, tstr,
				ip->exptime, filtnames[fenum], ip->Z, nstars);
	}

	/* get memory for stats and locations */
	ip->ssp = (StarStats *) malloc (nstars * sizeof(*ip->ssp));
	if (!ip->ssp) {
	    fprintf (stderr, "%s: No memory for star stats\n", base);
	    exit (1);
	}
	ip->rap = (double *) malloc (nstars * sizeof(*ip->rap));
	ip->decp = (double *) malloc (nstars * sizeof(*ip->decp));
	if (!ip->rap || !ip->decp) {
	    fprintf (stderr, "%s: No memory for star locations\n", base);
	    exit (1);
	}

	/* find good star stats and locations */
	for (i = ngood = 0; i < nstars; i++) {
	    StarStats *ssp = &ip->ssp[ngood];
	    if (starStats ((CamPixel *)fim.image, w, h, &stardef,
						xa[i], ya[i], ssp, msg) < 0) {
		fprintf (stderr, "%s: x=%d y=%d: %s\n", base, xa[i], ya[i],msg);
		continue;
	    }
	    if (xy2RADec (&fim, ssp->x, ssp->y, &ip->rap[i], &ip->decp[i]) < 0){
		fprintf (stderr, "%s: No WCS\n", base);
		exit(1);
	    }
	    ngood++;
	}

	/* ok */
	ip->nstars = ngood;
	free ((void *)xa);
	free ((void *)ya);
	free ((void *)ba);
	ip->ok = 1;
}

/* convert the filter named by FILTER in fip to one of our enum codes.
 * return 0 if ok else put why not in msg[] and return -1.
 */
static int
getFilter (FImage *fip, int *codep, char *msg)
{
	char filter[80];
	char f;

	if (getStringFITS (fip, "FILTER", filter) < 0) {
	    (void) sprintf (msg, "Image has no FILTER field.");
	    return (-1);
	}
	f = filter[0];

	if (islower(f))
	    f = (char)toupper(f);

	switch (f) {
	case 'B': *codep = BF; return(0);
	case 'V': *codep = VF; return(0);
	case 'R': *codep = RF; return(0);
	case 'I': *codep = IF; return(0);
	default:
	    (void) sprintf (msg, "Unknown filter: %c", filter[0]);
	    return (-1);
	}
}

/* get the airmass in fip by using the ELEVATION field.
 * return 0 if ok else put why not in msg[] and return -1.
 */
static int
getAirmass (FImage *fip, double *amp, char *msg)
{
	char elevation[80];
	double alt;

	if (getStringFITS (fip, "ELEVATION", elevation) < 0) {
	    (void) sprintf (msg, "Image has no ELEVATION field.");
	    return (-1);
	}

	if (scansex (elevation, &alt) < 0) {
	    (void) sprintf (msg, "Bad ELEVATION format: %s", elevation);
	    return (-1);
	}

	alt = degrad (alt);
	*amp = 1.0/sin(alt);

	return (0);
}

/* get the EXPTIME field.
 * return 0 if ok else put why not in msg[] and return -1.
 */
static int
getExptime (FImage *fip, double *expp, char *msg)
{
	if (getRealFITS (fip, "EXPTIME", expp) < 0) {
	    (void) sprintf (msg, "Image has no EXPTIME field.");
	    return (-1);
	}

	return (0);
}

/* get the JD field.
 * return 0 if ok else put why not in msg[] and return -1.
 */
static int
getJD (FImage *fip, double *jdp, char *msg)
{
	if (getRealFITS (fip, "JD", jdp) < 0) {
	    (void) sprintf (msg, "Image has no JD field.");
	    return (-1);
	}

	return (0);
}

/* make sure we have all four filters. if not, exit */
static void
check4Filters()
{
	if (!iparams[BF].ok) {
	    fprintf (stderr, "No B filter image\n");
	    exit(1);
	}
	if (!iparams[VF].ok) {
	    fprintf (stderr, "No V filter image\n");
	    exit(1);
	}
	if (!iparams[RF].ok) {
	    fprintf (stderr, "No R filter image\n");
	    exit(1);
	}
	if (!iparams[IF].ok) {
	    fprintf (stderr, "No I filter image\n");
	    exit(1);
	}
}

/* check the JD fields in the images for being within a small span. if ok,
 * return their average, else exit.
 */
static double
photJD ()
{
	double minjd, maxjd;

	minjd = maxjd = iparams[BF].jd;
	if (iparams[VF].jd < minjd) minjd = iparams[VF].jd;
	if (iparams[VF].jd > maxjd) maxjd = iparams[VF].jd;
	if (iparams[RF].jd < minjd) minjd = iparams[RF].jd;
	if (iparams[RF].jd > maxjd) maxjd = iparams[RF].jd;
	if (iparams[IF].jd < minjd) minjd = iparams[IF].jd;
	if (iparams[IF].jd > maxjd) maxjd = iparams[IF].jd;

	if (maxjd - minjd > IDT/24.0) {
	    fprintf (stderr, "Image JDs span more than %g hours\n", IDT);
	    exit (1);
	}

	return ((minjd + maxjd)/2);
}

/* fill in calconst[]: first look in calpath for an entry within PCDT of jd; if
 *   nothing there then try defpath.
 * if no luck, exit.
 */
static void
readCalConst(double jd)
{
	PCalConst pcc;
	char msg[1024];
	double jdf;
	char *filename;
	FILE *fp;

	/* try calpath */
	filename = NULL;
	fp = fopen (calpath, "r");
	if (fp) {
	    if (photReadCalConst (fp, jd, PCDT/24.0, &pcc, &jdf, msg) < 0)
		fprintf (stderr, "%s: Note: %s\n", calpath, msg);
	    else
		filename = calpath;
	    (void) fclose (fp);
	} else
	    fprintf (stderr, "%s: %s\n", calpath, strerror(errno));

	/* if calpath is unsuccessful, try defpath */
	if (!filename) {
	    fp = fopen (defpath, "r");
	    if (fp) {
		double B, V, R, I;
		if (photReadDefCalConst (fp, &pcc, &B, &V, &R, &I, msg) < 0) {
		    fprintf (stderr, "%s: %s\n", defpath, msg);
		    exit(1);	/* give up */
		} else
		    filename = defpath;
	    } else {
		fprintf (stderr, "%s: %s\n", defpath, strerror(errno));
		exit(1);	/* give up */
	    }
	}

	fillCalConst (&pcc);

	if (verbose) {
	    int i;

	    printf ("Corrections from %s:\n", filename);
	    for (i = BF; i <= IF; i++) {
		CalConst *ccp = &calconst[i];

		printf ("  %c: V0=%5.2f V0err=%5.2f kp=%5.2f kperr=%5.2f kpp=%5.2f kpperr=%5.2f\n",
					filtnames[i], ccp->V0, ccp->V0e,
					ccp->kp, ccp->kpe, ccp->kpp, ccp->kppe);
	    }
	}
}

/* fill all calconst[] from pccp */
static void
fillCalConst (PCalConst *pccp)
{
	CalConst *ccp;

	ccp = &calconst[BF];
	ccp->V0 = pccp->BV0; ccp->V0e = pccp->BV0e, ccp->kp = pccp->Bkp;
	ccp->kpe = pccp->Bkpe; ccp->kpp = pccp->Bkpp, ccp->kppe = pccp->Bkppe;

	ccp = &calconst[VF];
	ccp->V0 = pccp->VV0; ccp->V0e = pccp->VV0e, ccp->kp = pccp->Vkp;
	ccp->kpe = pccp->Vkpe; ccp->kpp = pccp->Vkpp, ccp->kppe = pccp->Vkppe;

	ccp = &calconst[RF];
	ccp->V0 = pccp->RV0; ccp->V0e = pccp->RV0e, ccp->kp = pccp->Rkp;
	ccp->kpe = pccp->Rkpe; ccp->kpp = pccp->Rkpp, ccp->kppe = pccp->Rkppe;

	ccp = &calconst[IF];
	ccp->V0 = pccp->IV0; ccp->V0e = pccp->IV0e, ccp->kp = pccp->Ikp;
	ccp->kpe = pccp->Ikpe; ccp->kpp = pccp->Ikpp, ccp->kppe = pccp->Ikppe;
}

/* build starsets: scan the BF star list, and build the starsets list with every
 * one which also has an entry in the VRI lists.
 */
static void
findCommonStars()
{
	int i, j, k;

	/* worst-case size is every star in the B image */
	starsets = (StarSet *) malloc (iparams[BF].nstars * sizeof(StarSet));
	if (!starsets) {
	    fprintf (stderr, "Can not malloc common star list\n");
	    exit (1);
	}

	/* for each B star */
	nstarsets = 0;
	for (i = 0; i < iparams[BF].nstars; i++) {
	    double bra = iparams[BF].rap[i];
	    double bdec = iparams[BF].decp[i];
	    int found[NF];

	    /* for each other filter */
	    for (j = VF; j <= IF; j++) {
		int nstars = iparams[j].nstars;
		double *rap = iparams[j].rap;
		double *decp = iparams[j].decp;

		/* find matching star */
		found[j] = 0;
		for (k = 0; k < nstars; k++) {
		    if (sameStar (bra, bdec, *rap++, *decp++) == 0) {
			starsets[nstarsets][j] = k;
			found[j] = 1;
			break;
		    }
		}
	    }

	    /* if found all VRI matches, add B and we have a new set */
	    if (found[VF] && found[RF] && found[IF]) {
		starsets[nstarsets][BF] = i;
		nstarsets++;
	    }
	}

	if (!nstarsets) {
	    fprintf (stderr, "No common stars\n");
	    exit (1);
	}
}

/* given two sky locations, return 0 if they are separated by <= STARSZ in
 * dec and ra, else -1.
 */
static int 
sameStar (
double ra0,
double dec0,
double ra1,
double dec1)
{
	double tmp;

	if (fabs(dec0 - dec1) > degrad(STARSZ/3600.0))
	    return (-1);

	tmp = fabs (ra0 - ra1);
	if (tmp > PI)
	    tmp = 2*PI - tmp;
	if (tmp*cos(dec0) > degrad(STARSZ/3600.0))
	    return (-1);

	return (0);
}

/* find the observed brightness of the given star normalized to the given
 * exposure time.
 */
static double
normalizedVobs (
StarStats *sp,	/* observed star */
double exp) 	/* exp time, seconds */
{
	double Vobs = -2.511886 * log10(sp->Src/exp);
	return (Vobs);
}

/* go through each starsets and compute and report magnitudes in each band. 
 * beware of bad values.
 */
static void
doReport()
{
	double V[NF];
	double err[NF];
	int i, j;
	int n;

	for (n = i = 0; i < nstarsets; i++) {
	    findMags (i, V, err);
	    for (j = 0; j < NF; j++)
		if (V[j] > 99.99 || V[j] < -99.99)
		    break;
	    if (j == NF) {
		printStar (n, V, err);
		n++;
	    }
	}

	if (verbose && n < i)
	    printf ("%d corrupt stars\n", i - n);
}

/* given a starsets index, find all the magnitude info */
static void
findMags (
int i,
double V[NF],
double err[NF])
{
	double bv, lastbv;

	/* find colorcorrected B and V by applying B-V until it converges */
	for (bv = 0, lastbv = 100; fabs(bv-lastbv) > 0.01; bv = V[BF]-V[VF]) {
	    findStarMag (i, BF, bv, &V[BF], &err[BF]);
	    findStarMag (i, VF, bv, &V[VF], &err[VF]);
	    lastbv = bv;
	}

	/* now find colorcorrected R and I */
	findStarMag (i, RF, bv, &V[RF], &err[RF]);
	findStarMag (i, IF, bv, &V[IF], &err[IF]);
}

/* given a star index (ie, into starsetsp[]), a filter enum and B-V correction,
 * compute and return the stars true magnitude and an estimate of the error.
 */
static void
findStarMag (
int star,
int fcode,
double bv,
double *Vp,
double *errp)
{
	IParams *ip = &iparams[fcode];
	StarStats *ssp = &ip->ssp[starsets[star][fcode]];
	CalConst *ccp = &calconst[fcode];
	double Vobs;

	Vobs = normalizedVobs (ssp, ip->exptime);
	*Vp = Vobs + ccp->V0 + ccp->kp*ip->Z + ccp->kpp*bv;
	*errp = 2.2/(((double)ssp->p - (double)ssp->Sky)/(double)ssp->rmsSky);
}

/* given a starsets index, print its report on one line */
static void
printStar (
int idx,
double V[NF],
double err[NF])
{
	char rastr[64], decstr[64];
	double ra = iparams[BF].rap[starsets[idx][BF]]; /* B good as any */
	double dec = iparams[BF].decp[starsets[idx][BF]];
	int i = 0;

	fs_sexa (rastr, radhr(ra), 2, 36000);
	fs_sexa (decstr, raddeg(dec), 3, 3600);

	if (verbose && i == 0) {
	    printf ("  N  RA         Dec     ");
	    for (i = 0; i < NF; i++) {
		char fn = filtnames[i];
	        printf ("  %c      %cerr  %cFMen %cFRti", fn, fn, fn, fn);
	    }
	    printf ("\n");
	}

	printf ("%3d %s %s", idx, rastr, decstr);
	for (i = 0; i < NF; i++) {
	    double fx = iparams[i].ssp[starsets[idx][i]].xfwhm;
	    double fy = iparams[i].ssp[starsets[idx][i]].yfwhm;
	    double fm = sqrt(fx*fy);
	    double fr = fy/fx;
	    printf (" %7.3f %5.3f %5.2f %5.3f", V[i], err[i], fm, fr);
	}
	printf ("\n");
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: fphotom.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
