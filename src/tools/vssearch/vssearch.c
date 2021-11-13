/* Perform automated variable star searches on a set of images.
 *  5 Sep 96 Elwood Downey
 * 31 Oct 97 Added -r switch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#ifdef _WIN32
#include <io.h>
#endif

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "photstd.h"
#include "strops.h"
#include "fits.h"
#include "wcs.h"

extern void heliocorr (double jd, double ra, double dec, double *hcp);
extern double delra (double dra);


#define	COMMENT	'#'

#define	MAXVOTES	5	/* max votes from each image for cal star */

#define	RSRCH		4	/* max radius to search for brightest pixel */

/* defaults in absense of a config file */
#define	LOCERR	5.0	/* star location tolerance, arc seconds */
#define	DEFFILT	'R'	/* default filter code */
#define	MAXERR	.1	/* max star stats error */
#define	MAXSERR	.1	/* max star mag estimate error */
#define	MAXADU	40000	/* max adu count to trust as a star */
#define	MINDEV	.1	/* min average deviation from mean */
#define	MINGOOD	3	/* star must be in at least this many images */

static StarDfn stardef = {RSRCH, ABSAP, SSHOW_MAXINAREA};
static double locerr = LOCERR;
static char deffilt = DEFFILT;
static double maxerr = MAXERR;
static double maxserr = MAXSERR;
static unsigned int maxadu = MAXADU;
static double mindev = MINDEV;
static int mingood = MINGOOD;

static int verbose;	/* >0 gives more info on stderr */
static int sflag;	/* >0 generate output in spreadsheet format */
static int rflag;	/* >0 user specified an explicit cal star .. cal_* */
static double cal_ra;	/* if rflag, user's RA of cal star*/
static double cal_dec;	/* if rflag, user's Dec of cal star*/
static double cal_mag=0;/* if rflag, user's assigned abs mag of cal star*/
static double cal_maglim=0;/* if rflag, user's limiting mag for report */

#define	JDBIAS	2450000./* subtract this from JD */

/* what we know about one image */
typedef struct {
    char fname[32];	/* base of filename */
    double exptime;	/* secs */
    double Z;     	/* airmass = 1/sin(alt) */
    double jd;		/* time of exposure, MHJD */
    int skip;		/* set if decide to skip this image afterall */
} ImageInfo;

/* malloced array of ImageInfos, one per image */
static ImageInfo *imagelist;
static int nimagelist;

/* what we know about one star on one image.
 * there is always one of these in a StarList for each imagelist but in will
 *   only be set of the image is actually in the given image.
 */
typedef struct {
    int in;		/* set if really in the image, then off if too noisy  */
    StarStats ss;	/* star stats */
    double ra, dec;	/* star position */

    /* these are computed for the final report */
    double m, e;	/* mag and err */
} StarInfo;

/* head of a list of nimagelist StarInfos for each unique star.
 */
typedef struct {
    StarInfo *sip;	/* array of nimagelist StarInfos */
    double ra, dec;	/* nominal star position */
    int nin;		/* number in the array that have in != 0 */

    /* these are computed for the final report */
    double q;		/* figure of merit -- higher means more likely a var */
    double dev;		/* average deviation from noise-weighted mean */
} StarList;

/* malloced array of StarLists, one per unique star with each sip having
 * nimagelist StarInfos. Thus, we effectively have a rectangular array of
 * StarInfos: nstarlist rows x nimagelist columns.
 */
static StarList *starlist;
static int nstarlist;

/* the result of all the above will be as follows:
 *
 *  starlist[]        nimagelist StarInfo's         imagelist[]
 *  ----------      ----------------------          -----------
 *  |        | ---> |  |  |  |  |  |  |  |          |         |
 *  |        | sip  ----------------------          |         |
 *  ----------      ----------------------          |         |
 *  |        | ---> |  |  |  |  |  |  |  |          -----------
 *  |        |      ----------------------          |         |
 *     ...                                          |         |
 *                  ----------------------          |         |
 *  |        | ---> |  |  |  |  |  |  |  |              ...
 *  |        |      ----------------------          |         |
 *  ----------                                      -----------
 *
 * there are always exactly nimagelist StarInfo's off each StarList->sip, and
 * there are also exactly nimagelist entries in the imagelist array.
 * One can think of the StarInfo lists as a 2d array, row index is a star,
 * column index is an image.
 */

/* the calibrator star */
static StarList *calstar;

static void usage (char *prog);
static int handleRFlag (char *av[]);
static int readFileList (char *fn, char ***fnp);
static void readConfigFile(char *fn);
static char *oneGoodLine (char *buf, int n, FILE *fp);
static void readImageFiles (char *fns[], int nfns);
static void readImage (char *filename);
static int getAirmass (FImage *fip, double *amp, char *msg);
static int getExptime (FImage *fip, double *expp, char *msg);
static int getJD (FImage *fip, double *jdp, char *msg);
static ImageInfo *newImage ();
static StarInfo *newStar (double ra, double dec);
static int sameStar (double ra0, double dec0, double ra1, double ra2);
static void findCalStar (void);
static void userCalStar (double ra, double dec);
static void tossNoCalImages(void);
static void findStats (void);
static void prReport();
static void prHeader();
static void sortStarList(void);
static int jdsort (const void *p1, const void *p2);
static void prSpreadsheet();

int
main (
int ac,
char *av[])
{
	char *progname = av[0];
	char *cfn = NULL;
	char *ffn = NULL;
	char **filenames;
	int nfilenames;

	/* crack arguments */
	for (av++; --ac > 0 && **av == '-'; av++) {
	    char *str = *av;
	    char c;

	    while ((c = *++str))
		switch (c) {
		case 'c':	/* config file */
		    if (ac < 2)
			usage (progname);
		    cfn = *++av;
		    ac--;
		    break;
		case 'f':	/* file of filenames */
		    if (ac < 2)
			usage (progname);
		    ffn = *++av;
		    ac--;
		    break;
		case 'r':	/* explicit cal star */
		    if (ac < 5 || handleRFlag (av) < 0)
			usage (progname);
		    av += 4;
		    ac -= 4;
		    rflag++;
		    break;
		case 's':	/* spreadsheet output format */
		    sflag++;
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
	 * must either be some filenames here or in a file, but not both.
	 */
	if ((ffn && ac>0) || (!ffn && ac==0))
	    usage (progname);

	/* set filenames from reading file of filenames, or use av[] */
	if (ffn) {
	    nfilenames = readFileList (ffn, &filenames);
	    if (nfilenames == 0) {
		fprintf (stderr, "No names in %s\n", ffn);
		exit(1);
	    }
	} else {
	    filenames = av;
	    nfilenames = ac;
	}

	/* read config file, if given */
	if (cfn)
	    readConfigFile(cfn);

	/* get imagelist and starlist started (to avoid realloc() hassle) */
	imagelist = (ImageInfo *) malloc (1);
	starlist = (StarList *) malloc (1);

	/* find the set of stars in all the images */
	readImageFiles(filenames, nfilenames);

	/* set a calibrator star */
	if (rflag)
	    userCalStar (cal_ra, cal_dec);
	else
	    findCalStar();
	if (verbose) {
	    char rstr[32], dstr[32];

	    fs_sexa (rstr, radhr(calstar->ra), 2, 36000);
	    fs_sexa (dstr, raddeg(calstar->dec), 2, 3600);
	    fprintf (stderr, "Cal star at %s %s\n", rstr, dstr);
	}

	/* skip all images which do not contain the calibrator star */
	tossNoCalImages();
	if (verbose) {
	    int i, n;

	    for (n = i = 0; i < nimagelist; i++)
		if (!imagelist[i].skip)
		    n++;
	    fprintf (stderr, "%d images, %d good\n", nimagelist, n);
	    fprintf (stderr, "%d unique stars\n", nstarlist);
	    for (n = i = 0; i < nstarlist; i++)
		n += starlist[i].nin;
	    fprintf (stderr, "%d total star instances\n", n);
	}

	/* fill in the derived starlist stats and find variable candidates */
	findStats();

	/* do the desired report */
	if (sflag)
	    prSpreadsheet();
	else
	    prReport();

	return (0);
}

static void
usage (char *prog)
{
    FILE *fp = stderr;

    fprintf(fp,"%s: [options] [files...]\n", prog);
    fprintf(fp,"Options:\n");
    fprintf(fp,"  -c file  config file. otherwise, defaults are:\n");
    fprintf(fp,"           1. Max star search radius: %d\n", RSRCH);
    fprintf(fp,"           2. Aperture radius: %d\n", ABSAP);
    fprintf(fp,"           3. Max star location tolerance, arc secs: %g\n", LOCERR);
    fprintf(fp,"           4. Filter code: %c\n", DEFFILT);
    fprintf(fp,"           5. Max star stats error: %g\n", MAXERR);
    fprintf(fp,"           6. Max star mag estimate error: %g\n", MAXSERR);
    fprintf(fp,"           7. Max star ADU count: %d\n", MAXADU);
    fprintf(fp,"           8. Min average dev from mean to qualify: %g\n", MINDEV);
    fprintf(fp,"           9. Min epochs a star must occupy: %d\n", MINGOOD);
    fprintf(fp,"  -f file  file of filenames. otherwise, give on command line.\n");
    fprintf(fp,"  -r RA Dec RefMag MagLim\n");
    fprintf(fp,"           Specify an explicit calibrator star:\n");
    fprintf(fp,"           RA:      H:M:S.S\n");
    fprintf(fp,"           Dec:     D:M:S\n");
    fprintf(fp,"           RefMag:  assigned absolute magnitude\n");
    fprintf(fp,"           MagLim:  dimmest abs magnitude to report\n");
    fprintf(fp,"  -s       generate spreadsheet output format\n");

    exit (1);
}

/* crack the 5 args associated with the -r flag.
 * av[] includes the -r.
 * return 0 if ok, -1 if trouble.
 */
static int
handleRFlag (char *av[])
{
	double tmp;

	if (scansex (*++av, &tmp) < 0) {
	    fprintf (stderr, "bad RA format with -r\n");
	    return (-1);
	}
	cal_ra = hrrad(tmp);

	if (scansex (*++av, &tmp) < 0) {
	    fprintf (stderr, "bad Dec format with -r\n");
	    return (-1);
	}
	cal_dec = degrad(tmp);

	cal_mag = atof (*++av);
	cal_maglim = atof (*++av);

	return (0);
}

/* open file fn[] and read each line, building an argv-style list at ***fnp.
 * return total number of good lines.
 * N.B. we skip lines which appear blank or that begin with COMMENT
 */
static int
readFileList (char *fn, char ***fnp)
{
	FILE *fp;
	char buf[256];
	char **argv;
	int n;

	fp = fopen (fn, "r");
	if (!fp) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    exit(1);
	}

	argv = malloc (1); /* just anything */
	n = 0;
	while (oneGoodLine (buf, sizeof(buf), fp)) {
	    int l = strlen (buf);
	    char *bufcpy;

	    buf[--l] = '\0';
	    bufcpy = malloc (l+1);
	    strcpy (bufcpy, buf);
	    argv = (char **) realloc ((char *)argv, (n+1)*sizeof(char *));
	    argv[n++] = bufcpy;
	}

	fclose (fp);
	*fnp = argv;
	return (n);
}

static void
readConfigFile(char *fn)
{
	char buf[1024];
	FILE *fp;

	/* open the file */
	fp = fopen (fn, "r");
	if (!fp) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    exit (1);
	}

	/* read enough good entries, or else */
	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	stardef.rsrch = atoi (buf);

	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	stardef.rAp = atoi (buf);

	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	locerr = atof (buf);

	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	deffilt = buf[0];

	if (islower(deffilt))
	    deffilt = toupper(deffilt);
	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	maxerr = atof(buf);

	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	maxserr = atof(buf);

	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	maxadu = atoi(buf);

	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	mindev = atof(buf);

	if (!oneGoodLine (buf, sizeof(buf), fp)) {
	    fprintf (stderr, "%s: short\n", fn);
	    exit (1);
	}
	mingood = atoi(buf);

	/* TODO: fuss if too _long_ ? */

	if (verbose) {
	    fprintf (stderr,"Max star search radius: %d\n", stardef.rsrch);
	    fprintf (stderr,"Aperture radius: %d\n", stardef.rAp);
	    fprintf (stderr,"Max star location tolerance, arc secs: %g\n",
								    locerr);
	    fprintf (stderr,"Filter code: %c\n", deffilt);
	    fprintf (stderr,"Max star stats error: %g\n", maxerr);
	    fprintf (stderr,"Max star mag estimate error: %g\n", maxserr);
	    fprintf (stderr,"Max star ADU count: %d\n", maxadu);
	    fprintf (stderr,"Min average deviation from mean: %g\n", mindev);
	    fprintf (stderr,"Min epochs a star must occupy: %d\n", mingood);
	}

	(void) fclose (fp);
}

/* read lines from fp until find one not blank and not starting with #.
 * if ok return buf, else NULL (just like fgets())
 */
static char *
oneGoodLine (char *buf, int n, FILE *fp)
{
	while (fgets (buf, n, fp))
	    if (!(buf[0] == COMMENT || buf[0] == '\n'))
		return (buf);
	return (NULL);
}

/* read all images and build up the imagelist and starlist arrays as we go */
static void
readImageFiles (char *fns[], int nfns)
{
	while (nfns-- > 0)
	    readImage (*fns++);
}

/* read the given FITS file. add image to imagelist and add stars to starlist.
 * we never exit -- and only complain at all to stderr if verbose.
 */
static void
readImage (char *filename)
{
	char msg[1024];
	char *base;
	FITSRow filter;
	int fd;
	FImage fim;
	ImageInfo *ip;
	int *xa, *ya;
	CamPixel *ba;
	int nstars, ngood;
	double Z, exptime, jd;
	int w, h;
	int i;

	/* open and read the image */
	base = basenm(filename);
#ifdef _WIN32
	fd = open (filename, O_RDONLY | O_BINARY);
#else
	fd = open (filename, O_RDONLY);
#endif
	if (fd < 0) {
	    if (verbose)
		fprintf (stderr, "%s: %s\n", filename, strerror(errno));
	    return;
	}
	if (readFITSHeader (fd, &fim, msg) < 0) {
	    if (verbose)
		fprintf (stderr, "%s: %s\n", base, msg);
	    close(fd);
	    return;
	}

	/* check that it's even the correct filter */
	if (getStringFITS (&fim, "FILTER", filter) < 0) {
	    if (verbose)
		fprintf (stderr, "%s: No FILTER\n", base);
	    return;
	}
	if (islower(filter[0]))
	    filter[0] = toupper(filter[0]);
	if (filter[0] != deffilt) {
	    if (verbose > 1)
		fprintf (stderr, "%s: FILTER is %c (not %c)\n", base,
							filter[0], deffilt);
	    return;
	}

	/* get airmass, exposure time and JD */
	if (getAirmass (&fim, &Z, msg) < 0) {
	    if (verbose)
		fprintf (stderr, "%s: %s\n", base, msg);
	    return;
	}
	if (getExptime (&fim, &exptime, msg) < 0) {
	    if (verbose)
		fprintf (stderr, "%s: %s\n", base, msg);
	    return;
	}
	if (getJD (&fim, &jd, msg) < 0) {
	    if (verbose)
		fprintf (stderr, "%s: %s\n", base, msg);
	    return;
	}

	/* image looks good -- add one more to imagelist */
	ip = newImage();
	strncpy (ip->fname, base, sizeof(ip->fname));
	ip->exptime = exptime;
	ip->Z = Z;
	ip->skip = 0;

	/* header was all good -- now reread to get the actual pixels */
	(void) lseek (fd, 0L, 0);
	resetFImage (&fim);
	if (readFITS (fd, &fim, msg) < 0) {
	    if (verbose)
		fprintf (stderr, "%s: %s\n", base, msg);
	    ip->skip = 1;
	    close(fd);
	    return;
	}
	close(fd);

	/* easy access to image size */
	w = fim.sw;
	h = fim.sh;

	/* find the stars in this image */
	nstars = findStars (fim.image, w, h, &xa, &ya, &ba);
	if (nstars <= 0) {
	    if (verbose)
		fprintf (stderr, "%s: no stars!\n", base);
	    if (nstars == 0) {
		free ((void *)xa);
		free ((void *)ya);
		free ((void *)ba);
	    }
	    resetFImage (&fim);
	    return;
	}

	/* add stats to nimagelist'th entry of the correct starlist.
	 * grow starlist if find a star we have not seen before.
	 */
	for (i = ngood = 0; i < nstars; i++) {
	    StarInfo *sip;
	    StarStats ss;
	    double ra, dec;
	    double err;

	    if (starStats ((CamPixel *)fim.image, w, h, &stardef, xa[i],
							ya[i], &ss, msg) < 0)
		continue;

	    if (xy2RADec (&fim, ss.x, ss.y, &ra, &dec) < 0) {
		if (verbose)
		    fprintf (stderr, "%s: No WCS\n", base);
		break; /* no point in going any more with this image */
	    }

	    if (i == 0) {
		/* adjust for Helio and JDBIAS once we know one star */
		double hc;

		heliocorr (jd, ra, dec, &hc);
		ip->jd = jd - hc - JDBIAS;
	    }

	    /* don't use it if too bright, too disperse, or too noisy */
	    if (ss.p > maxadu)
		continue;
	    if (ss.p <= ss.Sky)
		continue; /* hot pixel can make noise greater than peak */

	    /* pre apertuare photomerty -- 1/17/97
	    err = 2.2/(((double)ss.p - (double)ss.nmed)/(double)ss.nsd);
	    */

	    err = 2.2/(((double)ss.p - (double)ss.Sky)/ss.rmsSky);
	    if (err > maxerr)
		continue;

	    sip = newStar (ra, dec);
	    sip->ra = ra;
	    sip->dec = dec;
	    sip->ss = ss;
	    sip->in = 1;

	    ngood++;
	}

	if (verbose)
	    fprintf (stderr, "%s: %d stars\n", base, ngood);

	/* ok */
	free ((void *)xa);
	free ((void *)ya);
	free ((void *)ba);
	resetFImage (&fim);
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

/* add a new image.
 * extend imagelist and grow each array in starlist.
 */
static ImageInfo *
newImage()
{
	StarList *slp, *lastslp = &starlist[nstarlist];
	ImageInfo *newip;

	imagelist = (ImageInfo *) realloc ((void *)imagelist,
					(nimagelist + 1) * sizeof(ImageInfo));
	newip = &imagelist[nimagelist++];

	for (slp = starlist; slp < lastslp; slp++) {
	    slp->sip = (StarInfo *) realloc ((void *)slp->sip,
						nimagelist*sizeof(StarInfo));
	    memset ((void *)&(slp->sip[nimagelist-1]), 0, sizeof(StarInfo));
	}

	return (newip);
}

/* add a new star for imagelist[nimagelist-1].
 * look through starlist for one near ra/dec. if can't find, grow and add.
 * N.B. we assume newImage() has already insured all existing stars have
 *   at least nimagelist StarInfo's off sip.
 */
static StarInfo *
newStar (double ra, double dec)
{
	StarList *slp, *lastslp = &starlist[nstarlist];
	StarInfo *newsip;

	for (slp = starlist; slp < lastslp; slp++)
	    if (sameStar (ra, dec, slp->ra, slp->dec) == 0)
		break;

	if (slp == lastslp) {
	    starlist = (StarList *) realloc ((void *)starlist,
						(nstarlist+1)*sizeof(StarList));
	    slp = &starlist[nstarlist++];
	    memset ((void *)slp, 0, sizeof(StarList));
	    slp->sip = (StarInfo *) calloc (nimagelist, sizeof(StarInfo));
	    slp->ra = ra;
	    slp->dec = dec;
	}

	slp->nin++;
	newsip = &(slp->sip[nimagelist-1]);

	return (newsip);
}

/* given two sky locations, return 0 if they are separated by <= locerr in
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

	if (fabs(dec0 - dec1) > degrad(locerr/3600.0))
	    return (-1);

	tmp = delra (ra0 - ra1);
	if (tmp*cos(dec0) > degrad(locerr/3600.0))
	    return (-1);

	return (0);
}

static int sortimagen;

/* qsort-style function given 2 pointers-to-ints which are indeces into the
 * starlist array for image number sortimagen, sort by _decreasing_ peak pixel.
 */
static int
starsort (const void *p1, const void *p2)
{
	StarInfo *s1 = &(starlist[*(int *)p1].sip[sortimagen]);
	StarInfo *s2 = &(starlist[*(int *)p2].sip[sortimagen]);

	return (s2->ss.p - s1->ss.p);
}

/* for each image
 *   sort all its stars by brightness
 *   vote for the brightest MAXVOTES
 * set calstar to the StarInfo with the most votes.
 */
static void
findCalStar()
{
	int *votes = calloc (nstarlist, sizeof(int));
	int *stars = calloc (nstarlist, sizeof(int));
	int i, s;
	int maxv, maxi;

	for (i = 0; i < nimagelist; i++) {
	    int n;

	    for (n = s = 0; s < nstarlist; s++)
		if (starlist[s].sip[i].in)
		    stars[n++] = s;
	    if (n > 1) {
		sortimagen = i;
		qsort ((void  *)stars, n, sizeof(int), starsort);
		for (s = 0; s < MAXVOTES; s++)
		    votes[stars[s]]++;
	    }
	}

	maxi = maxv = -1;
	for (i = 0; i < nstarlist; i++)
	    if (votes[i] > maxv) {
		maxv = votes[i];
		maxi = i;
	    }

	calstar = &starlist[maxi];

	free ((void *)votes);
	free ((void *)stars);
}

/* the user has specified a cal star -- just find it and set calstar.
 * exit if not in list.
 */
static void
userCalStar (double ra, double dec)
{
	StarList *slp, *lastslp = &starlist[nstarlist];

	for (slp = starlist; slp < lastslp; slp++)
	    if (sameStar (ra, dec, slp->ra, slp->dec) == 0) {
		calstar = slp;
		return;
	    }

	fprintf (stderr, "No star found at suggested calibrator position\n");
	exit(1);
}

/* scan all images and mark each to be skipped that does not contain the
 * calibrator star.
 */
static void
tossNoCalImages()
{
	int i;

	for (i = 0; i < nimagelist; i++)
	    if (!calstar->sip[i].in) {
		if (verbose)
		    fprintf (stderr, "%s: skipping -- no cal star\n", 
						    imagelist[i].fname);
		imagelist[i].skip = 1;
	    }
}

/* fill in the derived starlist stats */
static void
findStats()
{
	StarList *slp, *lslp = &starlist[nstarlist];

	for (slp = starlist; slp < lslp; slp++) {
	    double chisq, dev;
	    double VE, OE;
	    double Vbar;
	    int n, i;

	    /* skip stars not in enough images */
	    if (slp->nin < mingood)
		continue;

	    /* one pass to compute the mag and error of each star.
	     * decrement sip->nin if find star is too dim or too noisy.
	     */
	    for (i = 0; i < nimagelist; i++) {
		StarInfo *sip;
		StarInfo *calp;

		if (imagelist[i].skip)
		    continue;
		sip = &(slp->sip[i]);
		if (!sip->in)
		    continue;

		calp = &(calstar->sip[i]);

		starMag (&calp->ss, &sip->ss, &sip->m, &sip->e);
		if (sip->e <= 1e-3) /* just in case since we divide by e later*/
		    sip->e = 1e-3;
		sip->in = (sip->e <= maxserr)
				&& (!rflag || cal_mag + sip->m < cal_maglim);
		if (!sip->in)
		    slp->nin--;
		    
	    }

	    /* skip stars not clean enough in enough images */
	    if (slp->nin < mingood) 
		continue;

	    /* another pass to build up the sums for the noise-weighted mean */
	    VE = OE = 0.0;
	    for (i = 0; i < nimagelist; i++) {
		ImageInfo *iip = &imagelist[i];
		StarInfo *sip;

		if (iip->skip)
		    continue;
		sip = &(slp->sip[i]);
		if (!sip->in)
		    continue;

		VE += sip->m / sip->e;
		OE += 1.0 / sip->e;
	    }

	    /* noise-weighted mean */
	    Vbar = VE/OE;

	    /* another pass to build up the sums for the Q and DEV */
	    dev = chisq = 0.0;
	    for (n = i = 0; i < nimagelist; i++) {
		ImageInfo *iip = &imagelist[i];
		StarInfo *sip;
		double t;

		if (iip->skip)
		    continue;
		sip = &(slp->sip[i]);
		if (!sip->in)
		    continue;

		t = (sip->m - Vbar)/sip->e;
		chisq += t*t;
		dev += fabs(sip->m - Vbar);
		n++;
	    }

	    if (n == 0) {
		slp->q = 0.0;
		slp->dev = 0.0;
	    } else {
		/* figure of merit based on mean squared error */
		slp->q = sqrt(chisq/n);

		/* average deviation from the noise-weighted mean */
		slp->dev = dev/n;
	    }
	}
}

static void
prReport()
{
	StarList *slp, *lslp = &starlist[nstarlist];

	prHeader();

	/* sort starlist by decreasing q */
	sortStarList();

	for (slp = starlist; slp < lslp; slp++) {
	    char rstr[32], dstr[32];
	    int i;

	    if (slp != calstar && (slp->nin < mingood || slp->dev < mindev))
		continue;

	    printf ("%-3d", slp-starlist);
	    printf (" %5.2f", slp->q);
	    printf (" %2d", slp->nin);
	    fs_sexa (rstr, radhr(slp->ra), 2, 36000);
	    fs_sexa (dstr, raddeg(slp->dec), 3, 3600);
	    printf (" %s %s", rstr, dstr);

	    for (i = 0; i < nimagelist; i++) {
		StarInfo *sip;

		if (imagelist[i].skip)
		    continue;

		sip = &(slp->sip[i]);
		if (sip->in)
		    printf ("|%5.2f %4.2f  ", sip->m+(rflag?cal_mag:0), sip->e);
		else
		    printf ("|            ");
	    }
	    printf ("\n");
	}
}

static void
prHeader()
{
	int i;

	printf ("Filename                         ");
	for (i = 0; i < nimagelist; i++) {
	    ImageInfo *ip = &imagelist[i];
	    if (ip->skip)
		continue;
	    printf ("|%12.12s", ip->fname);
	}
	printf ("\n");

	printf ("HelioJD-2450000                  ");
	for (i = 0; i < nimagelist; i++) {
	    ImageInfo *ip = &imagelist[i];
	    if (ip->skip)
		continue;
	    printf ("|%10.4f  ", ip->jd);
	}
	printf ("\n");

	printf ("Airmass                          ");
	for (i = 0; i < nimagelist; i++) {
	    ImageInfo *ip = &imagelist[i];
	    if (ip->skip)
		continue;
	    printf ("|%8.2f    ", ip->Z);
	}
	printf ("\n");

	printf ("Exposure,secs                    ");
	for (i = 0; i < nimagelist; i++) {
	    ImageInfo *ip = &imagelist[i];
	    if (ip->skip)
		continue;
	    printf ("|%8.2f    ", ip->exptime);
	}
	printf ("\n");

	printf ("I    Q     N RA          Dec     ");
	for (i = 0; i < nimagelist; i++) {
	    ImageInfo *ip = &imagelist[i];
	    if (ip->skip)
		continue;
	    printf ("| V    Verr  ");
	}
	printf ("\n");
}

/* qsort-style function to compare 2 StarLists for nice printing. */
static int
starlstsort (const void *p1, const void *p2)
{
	StarList *sl1 = (StarList *)p1;
	StarList *sl2 = (StarList *)p2;

	/* sort by decreasing q */
	if (sl1->q > sl2->q)
	    return (-1);
	else if (sl1->q < sl2->q)
	    return (1);
	return (0);
}

/* sort stars into an order most suitable for printing.
 * then update calstar and put it at the top of the list
 */
static void
sortStarList()
{
	double calq;

	/* force cal star to sort to top */
	calq = calstar->q;
	calstar->q = 1e20;

	qsort ((void *)starlist, nstarlist, sizeof(StarList), starlstsort);

	/* then put back its real q (should be 0 but just in case) */
	calstar = &starlist[0];
	calstar->q = calq;
}

/* qsort-style function given 2 pointers-to-ints which are indeces into the
 * imagelist array sort by increasing jd.
 */
static int
jdsort (const void *p1, const void *p2)
{
	ImageInfo *s1 = &(imagelist[*(int *)p1]);
	ImageInfo *s2 = &(imagelist[*(int *)p2]);

	if (s1->jd < s2->jd)
	    return (-1);
	else if (s1->jd > s2->jd)
	    return (1);
	return (0);
}

/* generate spreadsheet output report */
static void
prSpreadsheet()
{
	int *iljd;
	int i;

	/* sort an array of indeces into the imagelist according to increasing
	 * jd. we don't just sort the imagelist because we need to get at the
	 * entries in each starlist in the new order too.
	 */
	iljd = (int *) malloc (nimagelist * sizeof(int));
	for (i = 0; i < nimagelist; i++)
	    iljd[i] = i;
	qsort ((void *)iljd, nimagelist, sizeof(int), jdsort);

	/* sort the starlist too just so the ordinals match the other format */
	sortStarList();

	/* put up a simple header.
	 * don't label columns that won't have any valid entries.
	 */
	printf ("  MHJD    ");
	for (i = 0; i < nstarlist; i++) {
	    if (starlist[i].nin < mingood || starlist[i].dev < mindev)
		continue;
	    printf (" V%04d E%03d", i, i);
	}
	printf ("\n");

	/* make the table */
	/* for each image */
	for (i = 0; i < nimagelist; i++) {
	    int imidx = iljd[i];
	    ImageInfo *ip = &imagelist[imidx];
	    StarList *slp, *lslp = &starlist[nstarlist];

	    if (ip->skip)
		continue;

	    printf ("%10.4f", ip->jd);

	    /* for each star */
	    for (slp = starlist; slp < lslp; slp++) {
		StarInfo *sip = &slp->sip[imidx];
		double m, e;

		if (slp->nin < mingood || slp->dev < mindev)
		    continue;

		if (!sip->in) {
		    m = 99.99;
		    e =  9.99;
		} else {
		    m = sip->m + (rflag ? cal_mag : 0);
		    e = sip->e;
		}
		printf (" %5.2f %4.2f", m, e);
	    }
	    printf ("\n");
	}

	free ((void *)iljd);
}
