#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "preferences.h"

extern void eclipses (double mjdstart, int nminima, char *name, double ra,
    double dec, double p, double mjd0);

static void usage (char *p);
static void findEclipses (void);
static int crack_line (char buf[], char name[], double *rap, double *decp,
    double *pp, double *mjd0p);

/* program options and defaults */
static char def_starfile[] = "ecl-bin.txt";
static char *starfile;
static char *startdate;
static char *starname;
static int nminima = 1;
int showall;


int
main (int ac, char *av[])
{
	char *progname = av[0];
	char *str;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'a':	/* show all */
		    showall++;
		    break;
		case 'd':	/* alternate date */
		    if (startdate) {
			fprintf (stderr, "Only one -d is allowed.\n");
			usage(progname);
		    }
		    if (ac < 1)
			usage(progname);
		    startdate = *++av;
		    ac--;
		    break;
		case 'f':	/* alternate star list file */
		    if (starfile) {
			fprintf (stderr, "Only one -f is allowed.\n");
			usage(progname);
		    }
		    if (ac < 1)
			usage(progname);
		    starfile = *++av;
		    ac--;
		    break;
		case 'n':	/* number of minima */
		    if (ac < 1)
			usage(progname);
		    nminima = atoi (*++av);
		    ac--;
		    break;
		case 's':	/* specific star to report */
		    if (starname) {
			fprintf (stderr, "Only one -s is allowed.\n");
			usage(progname);
		    }
		    if (ac < 1)
			usage(progname);
		    starname = *++av;
		    ac--;
		    break;
		default: usage(progname); break;
		}
	}

	/* now there are ac remaining args starting at av[0] */
	if (ac > 0)
	    usage (progname);

	/* use default star file if not overridden */
	if (!starfile)
	    starfile = def_starfile;

	/* do it */
	findEclipses();

	return (0);
}

static void
usage (char *p)
{
    FILE *fp = stderr;

    fprintf (fp, "Usage: %s [options]\n", p);
    fprintf (fp, "   -a:          show all (not subject to dusk/dawn/alt/HA).\n");
    fprintf (fp, "   -d m/d/y:    starting date (y is full year, as in 1996); default is today.\n");
    fprintf (fp, "   -f starfile: alternate star list; default is %s\n", def_starfile);
    fprintf (fp, "   -n minima:   number of consecutive minima to report; default is 1.\n");
    fprintf (fp, "   -s starname: name of star; default is all stars in starfile.\n");

    exit (1);
}

static void
findEclipses()
{
	double mjdstart;
	FILE *fp;
	char buf[1024];

	/* open the star file */
	fp = fopen (starfile, "r");
	if (!fp) {
	    perror (starfile);
	    exit (1);
	}

	/* establish the starting JD from the date */
	if (startdate) {
	    /* crack user's value */
	    int m, y;
	    double d;

	    f_sscandate (startdate, PREF_MDY, &m, &d, &y);
	    cal_mjd (m, d, y, &mjdstart);
	} else {
	    /* now */
	    struct tm *tmp;
	    time_t t;

	    time (&t);
	    tmp = gmtime (&t);
	    cal_mjd (tmp->tm_mon+1, (double)(tmp->tm_mday), tmp->tm_year+1900,
								    &mjdstart);
	}

	/* scan file stars and report */
	while (fgets (buf, sizeof(buf), fp)) {
	    char name[1024];	/* star name */
	    double ra, dec;	/* J2000 location, rads */
	    double p;		/* period, days */
	    double mjd0;	/* mjd of a minima */

	    if (crack_line (buf, name, &ra, &dec, &p, &mjd0) < 0)
		continue;
	    if (!starname || strcasecmp (starname, name) == 0)
		eclipses (mjdstart, nminima, name, ra, dec, p, mjd0);
	}

	/* all finished */
	fclose (fp);
}

/* given a line from the star file, crack into its pieces.
 * return 0 if ok, else -1.
 */
static int
crack_line (
char buf[],	/* line from star file */
char name[],	/* star name */
double *rap,	/* J2000 RA, rads */
double *decp,	/* J2000 Dec, rads */
double *pp,	/* period, days */
double *mjd0p	/* mjd of a minimum */
)
{
	int rh, rm;
	double rs;
	char dsign;
	int dd, dm;
	double ds;

	/* crack line into components */
	if (sscanf (buf, "%s %d %d %lf %c%d %d %lf %lf %lf", name,
			&rh, &rm, &rs, &dsign, &dd, &dm, &ds, pp, mjd0p) != 10)
	    return (-1);

	/* build ra and dec */
	*rap = hrrad(rs/3600.0 + rm/60.0 + rh);
	*decp = degrad(ds/3600.0 + dm/60.0 + dd);
	if (dsign == '-')
	    *decp = - *decp;

	/* convert JD to mjd */
	*mjd0p -= MJD0;

	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: predict.c,v $ $Date: 2003/04/15 20:48:38 $ $Revision: 1.1.1.1 $ $Name:  $"};
