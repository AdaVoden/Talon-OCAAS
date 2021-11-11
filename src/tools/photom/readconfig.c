#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "strops.h"
#include "misc.h"
#include "configfile.h"
#include "wcs.h"
#include "photom.h"

#define	MAXLOC		64	/* max length of star location string */
#define	MAXLINE		256	/* longest line */
#define	MAXFN		128	/* longest filename */

#define	FLIST		'*'	/* denote a filename of files in config file */

static void readConfigLine (char *cfn, FILE *fp);
static void readFiles (char *cfn, FILE *fp);
static void readStars (char *cfn, FILE *fp, char locs[MAXSTARS][MAXLOC]);
static void setPositions (char *cfn, char locs[MAXSTARS][MAXLOC]);
static void addFileList (char *fn);
static void addFile(char *fn);
static void addOneFile (char *buf);
static int readGoodLine (char buf[], int buflen, FILE *fp);
static void saveWanderer(char *cfn, char *buf);
static void crackCoords (char *cfn, char *coordbuf, double *rap, double *decp);
static void setWanderer (char *cfn, int f, int s, FImage *fip);
static int setStarLoc (char *cfn, int f, int s, FImage *fip, char loc[MAXLOC]);
static int setOffset (int f, FImage *f0p, FImage *f1p);
static int nominalRADec (FImage *fip, double ra, double dec, double *xp,
    double *yp);
static char *rMalloc (char *p, int n);

static int foundwanderer;	/* set if found a wanderer */
static double wjd0, wra0, wdec0;/* one wanderer location */
static double wjd1, wra1, wdec1;/* another wanderer location */

/* read the entire config file and create and fill in the files[] array.
 * this also sets the values for the globals nfiles and nstars.
 * we only capture here what is know from the config file; x/y locations are
 *   aligned later as necessary.
 * if there is a wanderer, it is at files[i].sl[0].
 */
void
readConfigFile (cfn)
char *cfn;
{
	char locs[MAXSTARS][MAXLOC];
	FILE *fp;

	/* open the config file */
	fp = fopen (cfn, "r");
	if (!fp) {
	    perror (cfn);
	    exit (1);
	}

	/* read the config file parts.
	 * these all exit on error.
	 */
	readConfigLine (cfn, fp);
	readFiles (cfn, fp);
	readStars (cfn, fp, locs);	/* includes WANDERER, if present */

	if (verbose)
	    printf ("rsrch=%d rAp=%d nfiles=%d nstars=%d\n", stardfn.rsrch,
						stardfn.rAp, nfiles, nstars);

	/* finished with config file */
	fclose (fp);

	/* compute the files[i].{sl,dx,dy} */
	setPositions (cfn, locs);
}

/* read and process the first line (the set params)
 * just exit if trouble.
 */
static void
readConfigLine (char *cfn, FILE *fp)
{
	char buf[MAXLINE];
	double chg;
	int rsrch, rAp;
	int n;

	if (readGoodLine (buf, sizeof(buf), fp) < 0) {
	    fprintf (stderr, "%s: config file is empty\n", cfn);
	    exit (1);
	}

	n = sscanf (buf, "%d %d %lf", &rsrch, &rAp, &chg);

	if (n == 3) {
	    fprintf (stderr, "%s: looks like an old-style config file.\n", cfn);
	    fprintf (stderr, "new style wants Rsearch and Raperture only.\n");
	    fprintf (stderr, "Raperture may be 0 for automatic.\n");
	    exit (1);
	}
	if (n != 2) {
	    fprintf(stderr,"%s:first line must specify Rsearch and Raperture\n",
									cfn);
	    fprintf (stderr, "Raperture may be 0 for automatic.\n");
	    exit (1);
	}

	stardfn.rsrch = rsrch;
	stardfn.rAp = rAp;
	stardfn.how = SSHOW_MAXINAREA;
}

/* find the FILES section and read file names up to the FIXED section.
 * or allow a filename of files if name is preceeded with FLIST.
 * build up a clean files[] array and increment nfiles as we go.
 * allow for ranges.
 */
static void
readFiles (char *cfn, FILE *fp)
{
	char buf[MAXLINE];

	if (readGoodLine (buf, sizeof(buf), fp) < 0
					 || strncasecmp (buf, "FILES:", 6)) {
	    fprintf (stderr, "%s: expecting FILES section.\n", cfn);
	    exit (1);
	}

	while (1) {
	    char fn[MAXFN];

	    if (readGoodLine (buf, sizeof(buf), fp) < 0) {
		fprintf (stderr, "%s: No FIXED section\n", cfn);
		exit (1);
	    }
	    if (strncasecmp (buf, "FIXED:", 6) == 0)
		break;
	    if (sscanf (buf, "%s", fn) != 1) {
		fprintf (stderr, "%s: filename expected: %s\n", cfn, buf);
		exit (1);
	    }
	    if (fn[0] == FLIST) {
		if (strlen (fn) < 2) {
		    fprintf (stderr, "%s: bad file indirect: %s\n", cfn, buf);
		    exit(1);
		}
		addFileList (fn+1);
	    } else
		addFile (fn);	/* grows files[] and increments nfiles */
	}

	if (nfiles <= 0) {
	    fprintf (stderr, "%s: no files\n", cfn);
	    exit (1);
	}
}

/* read up to MAXSTARS star locations in the FIXED section.
 * also handle the WANDERER section if we see that.
 * done when see EOF or the optional WANDERER section.
 */
static void
readStars (char *cfn, FILE *fp, char locs[MAXSTARS][MAXLOC])
{
	char buf[MAXLINE];

	for (nstars = 0; nstars < MAXSTARS; nstars++) {
	    if (readGoodLine (buf, sizeof(buf), fp) < 0)
		break;
	    if (strncasecmp (buf, "WANDERER:", 9) == 0) {
		foundwanderer = 1;
		saveWanderer(cfn, buf);
		nstars++;
		break;
	    } else
		strcpy (locs[nstars], buf);
	}
	
	if (readGoodLine (buf, sizeof(buf), fp) == 0) {
	    fprintf(stderr, "%s: Too many stars -- max is %d\n", cfn, MAXSTARS);
	    exit (1);
	}
	if (nstars <= 0) {
	    fprintf (stderr, "%d: no stars.\n", nstars);
	    exit (1);
	}
}

/* open each file, compute the files[i].sl and the dx dy fields.
 */
static void
setPositions (char *cfn, char locs[MAXSTARS][MAXLOC])
{
	int f, s;
	FImage f0, f1;
	FImage *f0p, *f1p;

	initFImage (&f0);
	initFImage (&f1);
	f0p = &f0;
	f1p = &f1;

	/* read into f1p; f0p is prior -- used to find alignment */
	for (f = 0; f < nfiles; f++) {
	    resetFImage (f1p);
	    if (readFITSfile (files[f].fn, 1, f1p) < 0)
		continue;

	    /* locs[i] is the RA/Dec string for star i.
	     * the wanderer is not in this list but is counted in nstars.
	     * we want the wanderer to be at position 0 of the files.sl array.
	     */
	    for (s = 0; s < nstars; s++)
		if (foundwanderer) {
		    if (s == 0)
			setWanderer (cfn, f, s, f1p);
		    else {
			if (setStarLoc (cfn, f, s, f1p, locs[s-1]) < 0)
			    continue;
		    }
		} else {
		    if (setStarLoc (cfn, f, s, f1p, locs[s]) < 0)
			continue;
		}

	    if (f > 0) {
		if (setOffset (f, f0p, f1p) < 0)
		    continue;
	    }

	    resetFImage (f0p);
	    f0p = f1p;
	    f1p = f1p == &f0 ? &f1 : &f0;
	}

	resetFImage (f1p);
}

/* fn contains the name of a file which contains names of files or file ranges.
 * open the file, read out all the entries and call addFile() for each.
 * exit if trouble.
 */
static void
addFileList (char *fn)
{
	int nfiles;
	char **fnames;
	int i;

	nfiles = readFilenames (fn, &fnames);
	if (nfiles <= 0) {
	    fprintf (stderr, "No files in %s\n", fn);
	    exit(1);
	}

	for (i = 0; i < nfiles; i++) {
	    addFile (fnames[i]);
	    free (fnames[i]);
	}
	free ((void *)fnames);
}

/* add the file[s] named in fn and expand files[] array for that many more.
 * also increment nfiles by the new amount added.
 * if fn contains a '-' it is a range:
 *    ranges must be of the form Xaa-bb.fts, where:
 *      X is anything;
 *      aa-bb is a starting and ending hex range, such as 10-a0.
 * else it's just a single file and we add exactly one to files.
 */
static void
addFile (char *fn)
{
	char *cp;	/* pointer into fn at right-most '-', if any */
	int first, last;
	int nnew;
	char *suffix;
	int i;

	/* a '-' means a range.
	 * if no '-' then it's just a single simple file name.
	 */
	cp = strrchr (fn, '-');
	if (!cp) {
	    addOneFile (fn);
	    return;
	}

	/* extract the range info */
	if (sscanf (cp - 2, "%x-%x", &first, &last) != 2) {
	    fprintf (stderr, "Bad file range format in %s\n", fn);
	    exit (1);
	}
	nnew = last - first + 1;
	if (nnew <= 0) {
	    fprintf (stderr, "Bad file range in %s\n", fn);
	    exit (1);
	}
	if (nnew > 0xff) {
	    fprintf (stderr, "Preposterous number of files in range: %s\n", fn);
	    exit (1);
	}

	/* make fn contain a legal string of just the fixed leading
	 * portion of the filename. also, make suffix point to the
	 * trailing fixed portion.
	 */
	cp[-2] = '\0';
	suffix = cp + 3;

	/* create each filename in the range, adding to files array */
	for (i = 0; i < nnew; i++) {
	    char fni[1024];
	    sprintf (fni, "%s%x%s", fn, first+i, suffix);
	    addOneFile (fni);
	}
}

/* add one entry to the files[] array, and set fn to copy of newfn.
 */
static void
addOneFile (newfn)
char *newfn;
{
	char *mem;

	/* expand files array by 1 */
	files = (FileInfo *)rMalloc ((char *)files,(nfiles+1)*sizeof(FileInfo));
	memset ((char *)(&files[nfiles]), 0, sizeof(FileInfo));

	/* malloc copy of fn */
	mem = rMalloc (NULL, strlen(newfn) + 1);
	strcpy (mem, newfn);
	files[nfiles].fn = mem;

	/* inc nfiles */
	nfiles++;
}

/* read lines from fp until read one that does not begin with '#' or 0 len.
 * we also trim off the trailing '\n'.
 * return 0 if ok, else -1.
 */
static int
readGoodLine (buf, buflen, fp)
char buf[];
int buflen;
FILE *fp;
{
	do {
	    if (fgets (buf, buflen, fp) == NULL)
		return (-1);
	} while (buf[0] == '#' || buf[0] == '\n');
	buf[strlen(buf)-1] = '\0';
	return (0);
}

/* crack the WANDERER line and save in the w globals.
 */
static void
saveWanderer(char *cfn, char *buf)
{
	FImage fimage;
	char fn0[MAXFN], fn1[MAXFN];
	char ra0[64], dec0[64];
	char ra1[64], dec1[64];
	char coordbuf[128];
	int i;

	if (sscanf (buf, "%*s %s %s %s %s %s %s", fn0, ra0, dec0, 
							fn1, ra1, dec1) != 6) {
	    fprintf (stderr, "%s: bad WANDERER format: %s\n", cfn , buf);
	    exit (1);
	}

	/* add each filename to list if not already on it */
	for (i = 0; i < nfiles; i++)
	    if (strcasecmp (files[i].fn, fn0) == 0)
		break;
	if (i == nfiles)
	    addFile (fn0);
	for (i = 0; i < nfiles; i++)
	    if (strcasecmp (files[i].fn, fn1) == 0)
		break;
	if (i == nfiles)
	    addFile (fn1);

	/* dig out the JD of fn0 and crack the coords */
	initFImage (&fimage);
	if (readFITSfile (fn0, 1, &fimage) < 0)
	    exit(1);
	if (getRealFITS (&fimage, "JD", &wjd0) < 0) {
	    fprintf (stderr, "%s: no JD in %s\n", cfn, fn0);
	    exit(1);
	}
	sprintf (coordbuf, "%s %s", ra0, dec0);
	crackCoords (cfn, coordbuf, &wra0, &wdec0);

	/* dig out the JD of fn1 and crack the coords */
	resetFImage (&fimage);
	if (readFITSfile (fn1, 1, &fimage) < 0)
	    exit(1);
	if (getRealFITS (&fimage, "JD", &wjd1) < 0) {
	    fprintf (stderr, "%s: no JD in %s\n", cfn, fn1);
	    exit(1);
	}
	sprintf (coordbuf, "%s %s", ra1, dec1);
	crackCoords (cfn, coordbuf, &wra1, &wdec1);

	resetFImage (&fimage);

	/* do something to help out interpolation if wraps at 0H RA.
	 * we assume that if the two locations differ by more than 12 hours
	 * they have actually just crossed through 0. in which case we
	 * move the smaller of the two ahead by 2*PI. 
	 * N.B. This approach assumes the interpolator will confine its results
	 *   to 0..2*PI.
	 */
	if (fabs(wra1 - wra0) > PI) {
	    if (wra1 < wra0)
		wra1 += 2*PI;
	    else
		wra0 += 2*PI;
	}
}

/* take a string of "ra dec" and convert to ra dec in rads.
 * line can be of the form "HH:MM:SS.SS DD:MM:SS" or "degs_ra degs_dec".
 */
static void
crackCoords (char *cfn, char *coordbuf, double *rap, double *decp)
{
	char *rasexa, *decsexa;	/* set if we find ':' in each component */
	char rastr[64], decstr[64];

	/* find first white space and use to delimited RA portion */
	if (sscanf (coordbuf, "%s %s", rastr, decstr) != 2) {
	    fprintf (stderr, "%s: Bad RA/Dec format: '%s'\n", cfn, coordbuf);
	    exit (1);
	}

	/* see what we have */
	rasexa = strchr (rastr, ':');
	decsexa = strchr (decstr, ':');
	if (rasexa && decsexa) {
	    if (scansex (rastr, rap) < 0) {
		fprintf (stderr, "%s: Bad RA format: '%s'\n", cfn, rastr);
		exit (1);
	    }
	    *rap = hrrad(*rap);
	    if (scansex (decstr, decp) < 0) {
		fprintf (stderr, "%s: Bad Dec format: '%s'\n", cfn, decstr);
		exit (1);
	    }
	    *decp = degrad(*decp);
	} else if (!rasexa && !decsexa) {
	    *rap  = degrad(atof(rastr));
	    *decp = degrad(atof(decstr));
	} else {
	    fprintf (stderr, "%s: Bad format: %s %s\n", cfn, rastr, decstr);
	    exit (1);
	}
}

/* set files[f].sl[s].{x,y} from info in fip and the global w wanderer info.
 */
static void
setWanderer (char *cfn, int f, int s, FImage *fip)
{
	double ra, dec, jd;
	double x, y;

	if (getRealFITS (fip, "JD", &jd) < 0) {
	    fprintf (stderr, "%s: no JD in %s\n", cfn, files[f].fn);
	    exit(1);
	}

	ra =  wra0  - (wjd0 - jd) * (wra1  - wra0) /  (wjd1 - wjd0);
	dec = wdec0 - (wjd0 - jd) * (wdec1 - wdec0) / (wjd1 - wjd0);
	rdRange (&ra, &dec);

	if (RADec2xy (fip, ra, dec, &x, &y) < 0 &&
				nominalRADec (fip, ra, dec, &x, &y) < 0) {
	    fprintf (stderr, "%s: no WCS or RA/DEC fields in %s\n", cfn,
								files[f].fn);
	    exit (1);
	}
	files[f].sl[s].x = (int)floor(x + 0.5);
	files[f].sl[s].y = (int)floor(y + 0.5);
}

/* set files[f].sl[s].{x,y} from info in fip and loc */
static int
setStarLoc (char *cfn, int f, int s, FImage *fip, char loc[MAXLOC])
{
	double ra, dec;
	double x, y;

	crackCoords (cfn, loc, &ra, &dec);
	if (RADec2xy (fip, ra, dec, &x, &y) < 0 &&
				nominalRADec (fip, ra, dec, &x, &y) < 0) {
	    fprintf (stderr, "%s: no WCS or RA/DEC fields in %s\n", cfn,
	    							files[f].fn);
	    return (-1);
	}
	files[f].sl[s].x = (int)floor(x + 0.5);
	files[f].sl[s].y = (int)floor(y + 0.5);

	return (0);
}

/* using the nominal RA and DEC fields in fip, find the pixel location of the
 *   given location.
 * return 0 if ok, else -1.
 */
static int
nominalRADec (FImage *fip, double ra, double dec, double *xp, double *yp)
{
	FITSRow rastr, decstr;	/* RA and DEC fits fields */
	double nomra, nomdec;	/* nominal ra and dec of image center */
	double cdelt1, cdelt2;	/* ra step right and dec step down, degs/pixel*/
	int w, h;		/* image size */
	char errmsg[1024];

	if (getStringFITS (fip, "RA", rastr) < 0
				|| getStringFITS (fip, "DEC", decstr) < 0
				|| getRealFITS (fip, "CDELT1", &cdelt1) < 0
				|| getRealFITS (fip, "CDELT2", &cdelt2) < 0
				|| getNAXIS (fip, &w, &h, errmsg) < 0)
	    return (-1);

	/* RA field is in hours */
	if (scansex (rastr, &nomra) < 0)
	    return (-1);
	nomra = hrrad (nomra);

	/* DEC field is in degrees */
	if (scansex (decstr, &nomdec) < 0)
	    return (-1);
	nomdec = degrad (nomdec);


	*xp = w/2 + (ra-nomra)/degrad(cdelt1)*cos(nomdec);
	*yp = h/2 + (dec-nomdec)/degrad(cdelt2);
	if (*xp < 0 || *xp >= w || *yp < 0 || *yp >= h)
	    return (-1);

	return (0);
}

/* set files[f].{dx,dy} from f0p to f1p
 */
static int
setOffset (int f, FImage *f0p, FImage *f1p)
{
	char msg[1024];

	if (checkWCSFITS (f0p, 0) < 0 || checkWCSFITS (f1p, 0) < 0) {
	    files[f].dx = files[f].dy = 0;
	} else if (align2WCS (f0p, f1p, &files[f].dx, &files[f].dy, msg) < 0) {
	    fprintf (stderr, "%s: %s\n", files[f].fn, msg);
	    return (-1);
	}

	return (0);
}

/* like realloc but calls malloc if pointer is NULL */
static char *
rMalloc (p, n)
char *p;
int n;
{
	char *mem;

	if (p)
	    mem = realloc (p, n);
	else
	    mem = malloc (n);
	if (!mem) {
	    fprintf (stderr, "Can not malloc %d\n", n);
	    exit (1);
	}

	return (mem);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: readconfig.c,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $"};
