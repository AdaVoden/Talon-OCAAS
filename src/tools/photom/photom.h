/* include file for photom.
 * this requires fits.h
 */

/* info to describe where we start looking for a "star" */
typedef struct {
    int x, y;	/* initial starting loc */
} StarLoc;

#define	MAXSTARS	2000	/* max stars per file we can handle */
#define	MFWHM		10	/* max full-width-half-max, pixels */

/* global variables */

/* name and info about each star in one file.
 * N.B. only the first nstars entries of sloc and ss are used.
 */
typedef struct {
    char *fn;			/* file name */
    double pixdc0;		/* PIXDC0 from header */
    StarLoc sl[MAXSTARS];	/* initial locations of stars in this file */
    StarStats ss[MAXSTARS];	/* stats for each star in this file */
    int dx, dy;			/* alignment added to this image to register it
				 * with its reference image
				 */
} FileInfo;
extern int nstars;	/* number of xy points to compute */
extern FileInfo *files;	/* malloced array of FileInfos */
extern int nfiles;	/* number of filenames to process */
extern StarDfn stardfn;	/* parameters to use when looking for a star */

extern int verbose;	/* >0 for tracing info to stderr */
extern int wantfwhm;	/* >0 if want fwhm reported */
extern int wantxy;	/* >0 if want xy reported */

/* global functions */
extern int readFITSfile (char *fn, int justheader, FImage *fip);
extern void readConfigFile (char *configfile);
extern void doAllImages (void);
extern void doCropping (void);
extern void printStarStats (StarStats *ssp);
extern void printReportHeader (FImage *fip);
extern void printStarReport (StarStats *s0, StarStats *s1);
extern void printHeader(void);
extern void printImageHeader (int n, FImage *fip);

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: photom.h,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $
 */
