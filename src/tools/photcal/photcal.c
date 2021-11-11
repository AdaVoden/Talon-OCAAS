/* main for photcal program.
 * takes a file with values of know Landolt star fields, compares to a set of
 * images taken of these fields, and produces a best-fit set of instrument,
 * airmass and color corrections.
 * See the README for complete details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "preferences.h"
#include "telenv.h"
#include "photstd.h"

#include "photcal.h"

#define	MAXME	0.3	/* max err in Vobs vs. model fit after 1st pass */

/* default directories and file pathnames */
static char imagedir_def[] = "archive/photcal";
static char *imagedir = imagedir_def;
static char reffn_def[] = "photcal.ref";
static char *reffn = reffn_def;
static char deffn_def[] = "photcal.def";
static char *deffn = deffn_def;
static char calfn_def[] = "photcal.out";
static char *calfn = calfn_def;
static char auxdir_def[] = "archive/photcal";
static char *auxdir = auxdir_def;

/* net pathnames */
static char *refpath;
static char *defpath;
static char *calpath;

/* kpp defaults if no defpath */
static double Bkpp = -0.46, Vkpp = -0.15, Rkpp = -0.02, Ikpp = -0.06;

/* time window, days */
static double djd = 0.5;

int verbose;		/* -v flag: generate more chitchat */
static int errreport;	/* -m flag: generate star magnitudes error report */

static void usage (char *p);
static char *dirfn (char *dir, char *fn);
static void read_defaults (void);
static double crack_date (char mdy[]);
static void photcal(double jd);
static void remove_outlyers (PStdStar *sp, int nsp, double V0[4], double kp[4]);
static void rm_ol (OneStar sp[], int np, OneStar *outp[], int nout);
static void print_report (FILE *fp, double jd, double *V0, double *V0err, 
    double *kp, double *kperr);
static void print_errreport (FILE *fp, PStdStar *sp, int nsp, double V0[4],
    double kp[4]);

int
main (int ac, char *av[])
{
	char *progname;
	char *str;
	double jd;

	progname = av[0];

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'c':	/* alternate calibration file */
		    if (ac < 1)
			usage (progname);
		    calfn = *++av;
		    ac--;
		    break;
		case 'd':	/* alternate defaults file */
		    if (ac < 1)
			usage (progname);
		    deffn = *++av;
		    ac--;
		    break;
		case 'i':	/* alternate image directory */
		    if (ac < 1)
			usage (progname);
		    imagedir = *++av;
		    ac--;
		    break;
		case 'k':	/* alternate kpp */
		    if (ac < 4)
			usage (progname);
		    Bkpp = atof (*++av);
		    Vkpp = atof (*++av);
		    Rkpp = atof (*++av);
		    Ikpp = atof (*++av);
		    ac -= 4;
		    break;
		case 'm':	/* mag error report */
		    errreport++;
		    break;
		case 'p':	/* alternate defaults dir */
		    if (ac < 1)
			usage (progname);
		    auxdir = *++av;
		    ac--;
		    break;
		case 'r':	/* alternate reference file */
		    if (ac < 1)
			usage (progname);
		    reffn = *++av;
		    ac--;
		    break;
		case 't':	/* time window, days */
		    if (ac < 1)
			usage (progname);
		    djd = atof (*++av);
		    ac--;
		    break;
		case 'v':	/* verbose */
		    verbose++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0].
	 * it should be the date.
	 */
	if (ac < 1)
	    usage(progname);

	/* set up net path names to things */
	refpath = dirfn (auxdir, reffn);
	defpath = dirfn (auxdir, deffn);
	calpath = dirfn (auxdir, calfn);
	imagedir = dirfn (imagedir, NULL);

	/* keep going on math errors */
	signal (SIGFPE, SIG_IGN);

	/* read defaults file */
	read_defaults();

	/* find jd of given date */
	jd = crack_date (av[0]);

	/* do it */
	photcal (jd);

	return (0);
}

static void
usage (char p[])
{
	FILE *fp = stderr;

    fprintf(fp,"%s: [options] mm/dd/yyyy\n", p);
    fprintf(fp,"Purpose: analyze images of standard stars to find photometric constants.\n");
    fprintf(fp,"Options:\n");
    fprintf(fp,"  -i dir:  directory of images; default is %s\n",imagedir_def);
    fprintf(fp,"  -p file: directory for def/ref/out files; default is %s\n",
								    auxdir_def);
    fprintf(fp,"  -d file: default k'' values filename; default is %s\n",
								deffn_def);
    fprintf(fp,"  -r file: field ids filename; default is %s\n", reffn_def);
    fprintf(fp,"  -c file: calibration filename to append to (- means stdout);\n");
    fprintf(fp,"           default is %s\n", calfn_def);
    fprintf(fp,"  -k B V R I: values of k'' for each color;\n");
    fprintf(fp,"           may be set from file using -d or built-in\n");
    fprintf(fp,"           defaults are: %g %g %g %g\n", Bkpp,Vkpp,Rkpp,Ikpp);
    fprintf(fp,"  -t days: +/- days image's JD may vary from given date;");
	    fprintf(fp," default is %g\n", djd);
    fprintf(fp,"  -v:      verbose: generates additional output\n");
    fprintf(fp,"  -m:      print name, Z, filter, observed mag and err for each star.\n");
    fprintf(fp,"\n");
    fprintf(fp,"Required:\n");
    fprintf(fp,"  mm/dd/yyyy: date for which constants are to be determined\n");
    fprintf(fp,"           d may be a real number; y is full year, eg, 1995\n");

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


/* read defpath to find default values for kpp.
 * if can not find it, use the built-in defaults.
 */
static void
read_defaults()
{
	char buf[1024];
	FILE *fp;

	fp = fopen (defpath, "r");
	if (fp) {
	    int bok = 0, vok = 0, rok = 0, iok = 0;

	    if (verbose)
		printf ("Reading [BVRI]kpp from %s:\n", defpath);

	    while (fgets (buf, sizeof(buf), fp)) {
		double v, v0, kp, kpp;
		char *bp;
		char f;

		/* skip leading whitespace */
		for (bp = buf; *bp == ' ' || *bp == '\t'; bp++)
		    continue;

		/* eat the line */
		if (sscanf (bp, "%c: %lf %lf %lf %lf",
						&f, &v, &v0, &kp, &kpp) == 5) {
		    switch (f) {
		    case 'B': Bkpp = kpp; bok = 1; break;
		    case 'V': Vkpp = kpp; vok = 1; break;
		    case 'R': Rkpp = kpp; rok = 1; break;
		    case 'I': Ikpp = kpp; iok = 1; break;
		    default: fprintf (stdout, "%s: bad filter: %c\n",
								defpath, f);
			break;
		    }
		}
	    }

	    (void) fclose (fp);

	    if (!bok) fprintf (stdout, "%s: missing B values\n", defpath);
	    if (!vok) fprintf (stdout, "%s: missing V values\n", defpath);
	    if (!rok) fprintf (stdout, "%s: missing R values\n", defpath);
	    if (!iok) fprintf (stdout, "%s: missing I values\n", defpath);
	} else
	    fprintf (stdout, "%s: %s\n", defpath, strerror(errno));

	if (verbose)
	    printf ("Bkpp=%g Vkpp=%g Rkpp=%g Ikpp=%g\n", Bkpp, Vkpp, Rkpp,Ikpp);
}

/* return JD of the given date, assumed to be in m/d/y format */
static double
crack_date (char mdy[])
{
	int m, y;
	double d;
	double jd, Mjd;

	f_sscandate (mdy, PREF_MDY, &m, &d, &y);
	cal_mjd (m, d, y, &Mjd);
	jd = Mjd + MJD0;
	if (verbose)
	    printf ("JD = %14.7f +/- %g\n", jd, djd);

	return (jd);
}

static void
photcal(double jd)
{
	double V0[4], V0err[4], kp[4], kperr[4];   /* results, in order BVRI */
	PStdStar *sp;
	FILE *fp;
	int nsp;

	/* open and read the calibration file */
	fp = fopen (refpath, "r");
	if (!fp) {
	    fprintf (stdout, "%s: %s\n", refpath, strerror(errno));
	    return;
	}
	nsp = photStdRead (fp, &sp);
	if (nsp < 0) {
	    fprintf (stdout, "%s: no memory\n", refpath);
	    fclose (fp);
	    return;
	}
	fclose(fp);
	if (verbose)
	    printf ("Read %d fields from %s\n", nsp, refpath);

	/* scan the imagedir and compute all the star data */
	if (scanDir (imagedir, jd, djd, sp, nsp) < 0) {
	    fprintf (stdout, "Fatal error reading images.\n");
	    photFree (sp, nsp);
	    return;
	}

	/* first iteration: solve for the best values using all stars */
	if (solve_V0_kp (sp, nsp, Bkpp, Vkpp, Rkpp, Ikpp, V0, V0err,
							    kp, kperr) < 0) {
	    fprintf (stdout, "Complete solution not found in first pass.\n");
	    return;
	}

	if (verbose) {
	    printf ("Solution after first pass:\n");
	    print_report (stdout, jd, V0, V0err, kp, kperr);
	}

	/* remove stars whose model predictions vary more than MAXME from
	 * observed
	 */
	remove_outlyers (sp, nsp, V0, kp);

	/* second iteration: solve for best values using just stars which
	 * remain after removing outlyers.
	 */
	if (solve_V0_kp (sp, nsp, Bkpp, Vkpp, Rkpp, Ikpp, V0, V0err,
							    kp, kperr) < 0) {
	    fprintf (stdout, "Complete solution not found in second pass.\n");
	    return;
	}

	/* append results to calpath -- watch for "-" to mean stdout */
	if (strcmp (calpath, "-") == 0)
	    fp = stdout;
	else {
	    fp = fopen (calpath, "a");
	    if (!fp) {
		fprintf (stdout, "%s: %s\n", calpath, strerror(errno));
		return;
	    }
	}

	if (errreport)
	    print_errreport (stdout, sp, nsp, V0, kp);

	print_report (fp, jd, V0, V0err, kp, kperr);

	if (fp != stdout) {
	    if (verbose)
		print_report (stdout, jd, V0, V0err, kp, kperr);
	    fclose (fp);
	}

}

/* compute the difference between the observed value and the value predicted
 * the model and remove all stars which yield a difference of more than MAXME.
 */
static void
remove_outlyers (PStdStar *stp, int nstp, double V0[4], double kp[4])
{
	PStdStar *laststp;

	/* for each standard star in field */
	for (laststp = stp + nstp; stp < laststp; stp++) {
	    double colcor;
	    OneStar *lastsp;
	    OneStar *sp;
	    OneStar **outlyers;
	    int noutlyers;

	    /* find and remove each outlyer in the B images */
	    outlyers = (OneStar **) malloc (stp->nB * sizeof(OneStar *));
	    noutlyers = 0;
	    colcor = Bkpp*(stp->Bm - stp->Vm);
	    lastsp = stp->Bp + stp->nB;
	    for (sp = stp->Bp; sp < lastsp; sp++) {
		double VobsModel = stp->Bm - V0[0] - kp[0]*sp->Z - colcor;
		double modlerr = fabs (sp->Vobs - VobsModel);
		if (modlerr > MAXME) {
		    printf ("%12s: discarding %s B Z=%7.5f ModlErr= %g\n",
					sp->fn, stp->o.o_name, sp->Z, modlerr);
		    outlyers[noutlyers++] = sp;
		}
	    }
	    rm_ol (stp->Bp, stp->nB, outlyers, noutlyers);
	    stp->nB -= noutlyers;
	    free ((char *)outlyers);

	    /* find and remove each outlyer in the V images */
	    outlyers = (OneStar **) malloc (stp->nV * sizeof(OneStar *));
	    noutlyers = 0;
	    colcor = Vkpp*(stp->Bm - stp->Vm);
	    lastsp = stp->Vp + stp->nV;
	    for (sp = stp->Vp; sp < lastsp; sp++) {
		double VobsModel = stp->Vm - V0[1] - kp[1]*sp->Z - colcor;
		double modlerr = fabs (sp->Vobs - VobsModel);
		if (modlerr > MAXME) {
		    printf ("%12s: discarding %s V Z=%7.5f ModlErr= %g\n",
					sp->fn, stp->o.o_name, sp->Z, modlerr);
		    outlyers[noutlyers++] = sp;
		}
	    }
	    rm_ol (stp->Vp, stp->nV, outlyers, noutlyers);
	    stp->nV -= noutlyers;
	    free ((char *)outlyers);

	    /* find and remove each outlyer in the R images */
	    outlyers = (OneStar **) malloc (stp->nR * sizeof(OneStar *));
	    noutlyers = 0;
	    colcor = Rkpp*(stp->Bm - stp->Vm);
	    lastsp = stp->Rp + stp->nR;
	    for (sp = stp->Rp; sp < lastsp; sp++) {
		double VobsModel = stp->Rm - V0[2] - kp[2]*sp->Z - colcor;
		double modlerr = fabs (sp->Vobs - VobsModel);
		if (modlerr > MAXME) {
		    printf ("%12s: discarding %s R Z=%7.5f ModlErr= %g\n",
					sp->fn, stp->o.o_name, sp->Z, modlerr);
		    outlyers[noutlyers++] = sp;
		}
	    }
	    rm_ol (stp->Rp, stp->nR, outlyers, noutlyers);
	    stp->nR -= noutlyers;
	    free ((char *)outlyers);

	    /* find and remove each outlyer in the I images */
	    outlyers = (OneStar **) malloc (stp->nI * sizeof(OneStar *));
	    noutlyers = 0;
	    colcor = Ikpp*(stp->Bm - stp->Vm);
	    lastsp = stp->Ip + stp->nI;
	    for (sp = stp->Ip; sp < lastsp; sp++) {
		double VobsModel = stp->Im - V0[3] - kp[3]*sp->Z - colcor;
		double modlerr = fabs (sp->Vobs - VobsModel);
		if (modlerr > MAXME) {
		    printf ("%12s: discarding %s I Z=%7.5f ModlErr= %g\n",
					sp->fn, stp->o.o_name, sp->Z, modlerr);
		    outlyers[noutlyers++] = sp;
		}
	    }
	    rm_ol (stp->Ip, stp->nI, outlyers, noutlyers);
	    stp->nI -= noutlyers;
	    free ((char *)outlyers);
	}
}

/* remove the nout outlyers in outp[] from the list in sp[] with *npp entries.
 * N.B. we assume every entry in outp[] is somewher in sp[].
 * N.B. caller must reduce count by exactly nout.
 */
static void
rm_ol (OneStar sp[], int np, OneStar *outp[], int nout)
{
	int in;		/* walk through sp[] */
	int out;	/* walk through sp[] at next available slot */
	int i;		/* walk through outp[] */

	for (in = out = 0; in < np; in++) {
	    for (i = 0; i < nout; i++)
		if (&sp[in] == outp[i])
		    break;
	    if (i == nout) {
		/* sp[in] is not in outp[] so keep it in sp[] */
		if (in > out)	/* avoid move onto itself */
		    sp[out] = sp[in];
		out++;
	    }
	}
}

static void
print_report (FILE *fp, double jd, double *V0, double *V0err, double *kp,
double *kperr)
{
	int m, y;
	double d;

	fprintf(fp, "\n");

	mjd_cal (jd - MJD0, &m, &d, &y);
	fprintf(fp, "%13.5f (M/D/Y: %d/%g/%d)\n", jd, m, d, y);

	fprintf(fp, "Flt V0      V0err   kp     kperr   kpp    kpperr\n");
	fprintf(fp, "B: %7.3f %6.3f %7.3f %6.3f %7.3f %6.3f\n", V0[0], V0err[0],
						    kp[0], kperr[0], Bkpp, 0.0);
	fprintf(fp, "V: %7.3f %6.3f %7.3f %6.3f %7.3f %6.3f\n", V0[1], V0err[1],
						    kp[1], kperr[1], Vkpp, 0.0);
	fprintf(fp, "R: %7.3f %6.3f %7.3f %6.3f %7.3f %6.3f\n", V0[2], V0err[2],
						    kp[2], kperr[2], Rkpp, 0.0);
	fprintf(fp, "I: %7.3f %6.3f %7.3f %6.3f %7.3f %6.3f\n", V0[3], V0err[3],
						    kp[3], kperr[3], Ikpp, 0.0);
}

static void
print_errreport (
FILE *fp,
PStdStar *stp0,
int nstp,
double V0[4],
double kp[4]
)
{
	PStdStar *stp, *laststp;
	double VobsModel;

	/* header */
	fprintf (fp, "Filename     Star           C C Z      V-Vobs   ColCor  ModlErr StarErr\n");

	/* print each observed value, sorted by color */
	for (stp = stp0, laststp = stp + nstp; stp < laststp; stp++) {
	    double colcor = Bkpp*(stp->Bm - stp->Vm);
	    OneStar *lastsp = stp->Bp + stp->nB;
	    OneStar *sp;

	    /* for each observed B value */
	    for (sp = stp->Bp; sp < lastsp; sp++) {
		fprintf (fp, "%-*s", 12, sp->fn);
		fprintf (fp, " %-*s", MAXNM, stp->o.o_name);
		fprintf (fp, " B 0");
		fprintf (fp, " %5.3f", sp->Z);
		fprintf (fp, " %7.3f", stp->Bm - sp->Vobs);
		fprintf (fp, " %7.3f", colcor);
		VobsModel = stp->Bm - V0[0] - kp[0]*sp->Z - colcor;
		fprintf (fp, " %7.3f", sp->Vobs - VobsModel);
		fprintf (fp, " %7.3f", sp->Verr);
		fprintf (fp, "\n");
	    }
	}

	/* for each standard star in field */
	for (stp = stp0, laststp = stp + nstp; stp < laststp; stp++) {
	    double colcor = Vkpp*(stp->Bm - stp->Vm);
	    OneStar *lastsp = stp->Vp + stp->nV;
	    OneStar *sp;

	    /* for each observed V value */
	    for (sp = stp->Vp; sp < lastsp; sp++) {
		fprintf (fp, "%-*s", 12, sp->fn);
		fprintf (fp, " %-*s", MAXNM, stp->o.o_name);
		fprintf (fp, " V 1");
		fprintf (fp, " %5.3f", sp->Z);
		fprintf (fp, " %7.3f", stp->Vm - sp->Vobs);
		fprintf (fp, " %7.3f", colcor);
		VobsModel = stp->Vm - V0[1] - kp[1]*sp->Z - colcor;
		fprintf (fp, " %7.3f", sp->Vobs - VobsModel);
		fprintf (fp, " %7.3f", sp->Verr);
		fprintf (fp, "\n");
	    }
	}

	/* for each standard star in field */
	for (stp = stp0, laststp = stp + nstp; stp < laststp; stp++) {
	    double colcor = Rkpp*(stp->Bm - stp->Vm);
	    OneStar *lastsp = stp->Rp + stp->nR;
	    OneStar *sp;

	    /* for each observed R value */
	    for (sp = stp->Rp; sp < lastsp; sp++) {
		fprintf (fp, "%-*s", 12, sp->fn);
		fprintf (fp, " %-*s", MAXNM, stp->o.o_name);
		fprintf (fp, " R 2");
		fprintf (fp, " %5.3f", sp->Z);
		fprintf (fp, " %7.3f", stp->Rm - sp->Vobs);
		fprintf (fp, " %7.3f", colcor);
		VobsModel = stp->Rm - V0[2] - kp[2]*sp->Z - colcor;
		fprintf (fp, " %7.3f", sp->Vobs - VobsModel);
		fprintf (fp, " %7.3f", sp->Verr);
		fprintf (fp, "\n");
	    }
	}

	/* for each standard star in field */
	for (stp = stp0, laststp = stp + nstp; stp < laststp; stp++) {
	    double colcor = Ikpp*(stp->Bm - stp->Vm);
	    OneStar *lastsp = stp->Ip + stp->nI;
	    OneStar *sp;

	    /* for each observed I value */
	    for (sp = stp->Ip; sp < lastsp; sp++) {
		fprintf (fp, "%-*s", 12, sp->fn);
		fprintf (fp, " %-*s", MAXNM, stp->o.o_name);
		fprintf (fp, " I 3");
		fprintf (fp, " %5.3f", sp->Z);
		fprintf (fp, " %7.3f", stp->Im - sp->Vobs);
		fprintf (fp, " %7.3f", colcor);
		VobsModel = stp->Im - V0[3] - kp[3]*sp->Z - colcor;
		fprintf (fp, " %7.3f", sp->Vobs - VobsModel);
		fprintf (fp, " %7.3f", sp->Verr);
		fprintf (fp, "\n");
	    }
	}
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: photcal.c,v $ $Date: 2003/04/15 20:48:35 $ $Revision: 1.1.1.1 $ $Name:  $"};
