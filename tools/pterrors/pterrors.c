/* given a list of images, generate a report of HA/Dec or Alt/Az pointing
 * errors. take into account an existing pointing model too, if any.
 * can also report raw telescope axes vs. HA/Dec for use with mntmodel.
 *
 * The following FITS header fields are required:
 *   C* WCS fields
 *   LATITUDE
 *   LONGITUDE
 *   JD
 *   WXTEMP	(or 10 if missing)
 *   WXPRES	(or 1010 if missing)
 *   RAWHENC	(for -t only)
 *   RAWDENC	(for -t only)
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "strops.h"
#include "fits.h"
#include "wcs.h"

static void usage (char *p);
static void setNow (Now *np, FImage *fip);
static void readModel (void);
static void applyModel(double ha, double dec, double *dhap, double *ddecp);
static void oneFile (char *fn);

static int aflag;		/* show Alt/Az */
static int cflag;		/* multiply HA/Az by cos(Dec/Alt) */
static int mflag;               /* flag to engage ad-hoc for mount model */
static int pflag;		/* prefix each report with filename */
static int tflag;		/* report telescope axes at each HA/Dec */
static int vflag;               /* verbose flag */
static int jflag;               /* prepend with JD */
static char *mfile;             /* name of file with mount model params */

static double HA1, HA2, HA3, HA4, HA5;		/* model coefficients */
static double DEC1, DEC2, DEC3, DEC4, DEC5;	/* model coefficients */

int
main (int ac, char *av[])
{
	char *progname = av[0];

	/* crack arguments as -abc or -a -b -c */
	for (av++; --ac > 0 && **av == '-'; av++) {
	    char *str = *av;
	    char c;
	    /* in the loop ac includes the current arg */
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'a':	/* print in alt/az */
		    aflag++;
		    break;
		case 'c':	/* report sky angle, not polar angle */
		    cflag++;
		    break;
		case 'j':	/* prepend JD */
		    jflag++;
		    break;
		case 'm':       /* account for mount model */
		   if (ac < 2)
		       usage (progname);
		   mflag++;
		   mfile = *++av;
		   --ac;
		   break;
		case 'p':	/* prefix with filename */
		    pflag++;
		    break;
		case 't':	/* report raw telescope axes */
		    tflag++;
		    break;
		case 'v':	/* verbose */
		    vflag++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0] */
	if (ac < 1)
	    usage (progname);

	/* read model, if desired */
	if (mflag) {
	    if (vflag)
		fprintf (stderr, "Reading %s:\n", mfile);
	    readModel();
	}

	/* process files */
	while (--ac >= 0)
	    oneFile (*av++);

	return (0);
}

static void
usage (char *p)
{
	fprintf(stderr, "%s: [options] *.ft[sh]\n", p);
	fprintf(stderr, "  -a  : print Alt/Az format\n");
	fprintf(stderr, "  -c  : multiply HA by cos(Dec) (or Az by cos(Alt) if -a) to form sky angle\n");
	fprintf(stderr, "  -j  : prefix each line with JD\n");
	fprintf(stderr, "  -m f: include model in file `f' -- see README\n");
	fprintf(stderr, "  -p  : prefix each line with filename\n");
	fprintf(stderr, "  -t  : print telescope axes at each HA/Dec\n");
	fprintf(stderr, "  -v  : verbose\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "HA/Dec Output format (default):\n");
	fprintf(stderr, "  Target HA, hours\n");
	fprintf(stderr, "  Target Dec, degrees\n");
	fprintf(stderr, "  Target HA - WCS HA, arcminutes (*cos(Dec) if -c)\n");
	fprintf(stderr, "  Target Dec - WCS Dec, arcminutes\n");
	fprintf(stderr, "Alt/Az Output format (-a):\n");
	fprintf(stderr, "  Target Az, degrees\n");
	fprintf(stderr, "  Target Alt, degrees\n");
	fprintf(stderr, "  Target Az - WCS Az, arcminutes (*cos(Alt) if -c)\n");
	fprintf(stderr, "  Target Alt - WCS Alt, arcminutes\n");
	fprintf(stderr, "Telescope Axes Output format (-t):\n");
	fprintf(stderr, "  WCS HA, rads\n");
	fprintf(stderr, "  WCS Dec, rads\n");
	fprintf(stderr, "  RAWHENC, rads from home\n");
	fprintf(stderr, "  RAWDENC, rads from home\n");
	exit (1);
}

/* fill np with stuff from fip */
static void
setNow (Now *np, FImage *fip)
{
	char buf[128];
	double tmp;

	memset ((void *)np, 0, sizeof(Now));

	if (getStringFITS (fip, "LATITUDE", buf) < 0) {
	    fprintf (stderr, "No LATITUDE field in image\n");
	    exit(1);
	}
	(void) scansex (buf, &tmp);
	lat = degrad (tmp);

	if (getStringFITS (fip, "LONGITUD", buf) < 0) {
	    fprintf (stderr, "No LONGITUD field in image\n");
	    exit(1);
	}
	(void) scansex (buf, &tmp);
	lng = degrad (tmp);

	if (getRealFITS (fip, "WXTEMP", &tmp) < 0)
	    tmp = 10;
	temp = tmp;

	if (getRealFITS (fip, "WXPRES", &tmp) < 0)
	    tmp = 1010;
	pressure = tmp;

	epoch = EOD;
}

/* read the file of model coefficients */
static void
readModel()
{
	FILE *fp;

	fp = fopen (mfile, "r");
	if (!fp) {
	    fprintf (stderr, "%s: %s\n", mfile, strerror(errno));
	    exit(1);
	}

	(void) fscanf (fp, "%lf", &HA1);
	(void) fscanf (fp, "%lf", &HA2);
	(void) fscanf (fp, "%lf", &HA3);
	(void) fscanf (fp, "%lf", &HA4);
	(void) fscanf (fp, "%lf", &HA5);
	(void) fscanf (fp, "%lf", &DEC1);
	(void) fscanf (fp, "%lf", &DEC2);
	(void) fscanf (fp, "%lf", &DEC3);
	(void) fscanf (fp, "%lf", &DEC4);
	(void) fscanf (fp, "%lf", &DEC5);
	if (feof(fp) || ferror(fp)) {
	    fprintf (stderr, "%s: %s\n", mfile, strerror(errno));
	    exit(1);
	}
	fclose (fp);

	if (vflag) {
	    fprintf (stderr, " HA1 = %9.5f\n", HA1);
	    fprintf (stderr, " HA2 = %9.5f\n", HA2);
	    fprintf (stderr, " HA3 = %9.5f\n", HA3);
	    fprintf (stderr, " HA4 = %9.5f\n", HA4);
	    fprintf (stderr, " HA5 = %9.5f\n", HA5);
	    fprintf (stderr, "DEC1 = %9.5f\n", DEC1);
	    fprintf (stderr, "DEC2 = %9.5f\n", DEC2);
	    fprintf (stderr, "DEC3 = %9.5f\n", DEC3);
	    fprintf (stderr, "DEC4 = %9.5f\n", DEC4);
	    fprintf (stderr, "DEC5 = %9.5f\n", DEC5);
	}
}

/* compute the model errors for the given location onto the existing errors 
 * and add them to any existing error values.
 * N.B. all our arguments are in rads, *dhap is a polar angle.
 */
static void
applyModel(double ha, double dec, double *dhap, double *ddecp)
{

	double dha, ddec;

	/* N.B. this model computes errors in arc minutes */

	dha = (HA1*sin(ha)-HA2*cos(ha))*sin(dec) + HA3 + HA4*cos(ha-0.2)
								+ HA5*tan(dec);
	dha /= fabs(dec)<PI/2 ? cos(dec) : 1e-6; /* want polar angle, not sky */
	ddec = DEC1*cos(ha) + DEC2*sin(ha) + DEC3 + DEC4*cos(dec+0.35)
								+ DEC5*tan(dec);

	*dhap += degrad(dha/60.);
	*ddecp += degrad(ddec/60.);
}

static void
oneFile (char *fn)
{
	FImage fimage, *fip = &fimage;
	Now now, *np = &now;
	char errmsg[1024];
	Obj obj;
	char str[128];
	double jd, ra, dec, ha;
	double wra, wdec;
	double dha, ddec;
	int nr, nc;
	int fd;

	/* read the file header */
	fd = open (fn, O_RDONLY);
	fn = basenm(fn);	/* better for error messages */
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	    return;
	}
	if (readFITSHeader (fd, fip, errmsg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, errmsg);
	    (void) close (fd);
	    return;
	}
	(void) close (fd);

	/* fill Now */
	setNow (np, fip);

	/* get time of image */
	if (getRealFITS (fip, "JD", &jd) < 0) {
	    fprintf (stderr, "%s: No JD field\n", fn);
	    resetFImage (fip);
	    return;
	}
	mjd = jd - MJD0;

	/* get the WCS (astrometric J2000) center: wra and wdec */
	if (getNAXIS (fip, &nr, &nc, errmsg) < 0) {
	    fprintf (stderr, "%s: %s\n", fn, errmsg);
	    resetFImage (fip);
	    return;
	}
	if (xy2RADec (fip, nc/2.0, nr/2.0, &wra, &wdec) < 0) {
	    fprintf (stderr, "%s: no WCS fields\n", fn);
	    resetFImage (fip);
	    return;
	}

	/* get the nominal J2000 pointing fields: ra, dec, ha */
	if (getStringFITS (fip, "RA", str) < 0) {
	    fprintf (stderr, "%s: no RA\n", fn);
	    resetFImage (fip);
	    return;
	}
	(void) scansex (str, &ra);
	ra = hrrad(ra);
	if (getStringFITS (fip, "DEC", str) < 0) {
	    fprintf (stderr, "%s: no DEC\n", fn);
	    resetFImage (fip);
	    return;
	}
	(void) scansex (str, &dec);
	dec = degrad(dec);

	/* compute the errors in rads, dha a polar angle */
	dha = wra - ra; 	/* dha = -dra */
	ddec = dec - wdec;

	/* now need _refracted_ telescope ha dec */
	memset ((void *)&obj, 0, sizeof(Obj));
	obj.o_type = FIXED;
	obj.f_RA = ra;
	obj.f_dec = dec;
	obj.f_epoch = J2000;
	obj_cir (np, &obj);
	aa_hadec (lat, obj.s_alt, obj.s_az, &ha, &dec);


	/* apply model, if desired */
	if (mflag)
	    applyModel (ha, dec, &dha, &ddec);

	if (aflag) {
	    /* alt/az format */
	    double alt0, az0, alt1, az1;
	    double alt, az, dalt, daz;

	    /* convert HA/Dec to Alt/Az, find differentials implicitly */
	    hadec_aa (lat, ha, dec, &alt0, &az0);
	    hadec_aa (lat, ha+dha, dec+ddec, &alt1, &az1);

	    alt = alt0;
	    az = az0;
	    dalt = alt1 - alt0;
	    daz = az1 - az0;
	    if (daz < -PI) daz += 2*PI;
	    if (daz >  PI) daz -= 2*PI;

	    /* convert daz to sky angle, if desired */
	    if (cflag)
		daz *= cos(alt);
	    if (pflag)
		printf ("%s ", fn);
	    if (jflag)
		printf ("%13.5f ", jd);
	    printf ("%10.5f %10.5f %7.2f %7.2f\n", raddeg(az), raddeg(alt),
					    raddeg(daz)*60, raddeg(dalt)*60);

	} else if (tflag) {
	    /* telescope axes format */
	    double rawhenc, rawdenc;
	    char object[128];

	    if (getRealFITS (fip, "RAWHENC", &rawhenc) < 0) {
		fprintf (stderr, "%s: No RAWHENC field\n", fn);
		resetFImage (fip);
		return;
	    }
	    if (getRealFITS (fip, "RAWDENC", &rawdenc) < 0) {
		fprintf (stderr, "%s: No RAWDENC field\n", fn);
		resetFImage (fip);
		return;
	    }
	    if (getStringFITS (fip, "OBJECT", object) < 0) {
		fprintf (stderr, "%s: No OBJECT field\n", fn);
		resetFImage (fip);
		return;
	    }

	    if (pflag)
		printf ("%s ", fn);
	    if (jflag)
		printf ("%15s @ %13.5f ", object, jd);
	    printf ("%10.7f %10.7f %10.7f %10.7f\n", ha, dec, rawhenc, rawdenc);
	} else {
	    /* HA/Dec format (default) */
	    /* convert dha to sky angle, if desired */
	    if (cflag)
		dha *= cos(dec);
	    if (pflag)
		printf ("%s ", fn);
	    if (jflag)
		printf ("%13.5f ", jd);
	    printf ("%10.5f %10.5f %7.2f %7.2f\n", radhr(ha), raddeg(dec),
					    raddeg(dha)*60, raddeg(ddec)*60);
	}

	resetFImage (fip);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pterrors.c,v $ $Date: 2003/04/15 20:48:38 $ $Revision: 1.1.1.1 $ $Name:  $"};
