/* convert among various time formats */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "strops.h"
#include "telenv.h"
#include "misc.h"

static void usage (void);
static void ts2U (char *ts);

static char *me;

int
main (int ac, char *av[])
{
	int d, h, m, s, M, Y;
	char *str;
	int l;

	me = basenm(av[0]);

	while ((--ac > 0) && ((*++av)[0] == '-')) {
	    char *s;
	    for (s = av[0]+1; *s != '\0'; s++)
		switch (*s) {
		case 't':
		    printf ("%s", timestamp(time(NULL)));
		    return (0);
		case 'T':
		    if (ac-- != 2)
			usage();
		    /* mjd was 25567.5 on 00:00:00 1/1/1970 UTC (UNIX' epoch) */
		    printf ("%s", timestamp (
			    (time_t)((atof(*++av)-MJD0-25567.5)*SPD + 0.5)));
		    return (0);
		case 'u':
		    if (ac-- != 2)
			usage();
		    printf ("%s", timestamp (atol(*++av)));
		    return (0);
		case 'U':
		    if (ac-- != 2)
			usage();
		    ts2U (*++av);
		    return (0);
		default:
		    usage();
		}
	}

	/* ac remaining args starting at av[0] */

	if (ac == 0) {
	    /* no args: print current jd */
	    double jd = mjd_now() + MJD0;
	    printf ("%13.5f\n", jd);
	    return (0);
	}

	/* now must have a time or JD */
	if (ac != 1)
	    usage ();

	str = av[0];
	l = strlen(str);

	if (l == 15 && strchr(str, '.') == &str[l-3]) {
	    /* assume MMDDhhmmYYYY.ss and convert to jd
	     *        012345678901234
	     */
	    double D, Mjd;

	    s = atoi(&str[13]);
	    str[12] = '\0';
	    Y = atoi(&str[8]);
	    str[8] = '\0';
	    m = atoi (&str[6]);
	    str[6] = '\0';
	    h = atoi (&str[4]);
	    str[4] = '\0';
	    D = atof (&str[2]);
	    str[2] = '\0';
	    M = atoi (&str[0]);

	    D += (s/3600. + m/60.0 + h)/24.0;
	    cal_mjd (M, D, Y, &Mjd);
	    printf ("%13.5f\n", Mjd + MJD0);
	    return (0);

	} else if (sscanf (str,"%4d%2d%2d%2d%2d%2d",&Y,&M,&d,&h,&m,&s) == 6) {
	    /* assume timestamp format and convert to jd */
	    double D, Mjd;

	    D = d + (s/3600. + m/60.0 + h)/24.0;
	    cal_mjd (M, D, Y, &Mjd);
	    printf ("%13.5f\n", Mjd + MJD0);
	    return (0);
	} else {
	    /* assume jd and convert to UT string */
	    char *asct;
	    double jd;
	    time_t t;

	    /* UNIX t is seconds since 00:00:00 1/1/1970 UTC on UNIX systems;
	     * mjd was 25567.5 then.
	     */
	    jd = atof (str);
	    t = ((jd - MJD0) - 25567.5)*24.*3600.;
	    asct = asctime(gmtime(&t));
	    printf ("%.19s UTC %s", asct, &asct[20]);
	    return (0);
	}

	usage ();
	return (1);
}

/* convert timestamp to UNIX time */
static void
ts2U (char *ts)
{
	int d, h, m, s, M, Y;

	if (sscanf (ts,"%4d%2d%2d%2d%2d%2d",&Y,&M,&d,&h,&m,&s) == 6) {
	    /* assume timestamp format and convert to jd */
	    double D, Mjd;
	    long t;

	    D = d + (s/3600. + m/60.0 + h)/24.0;
	    cal_mjd (M, D, Y, &Mjd);
	    t = (Mjd - 25567.5)*24.*3600.;
	    printf ("%ld\n", t);
	} else
	    usage();
	exit (0);
}

static void
usage (void)
{
    FILE *fp = stderr;
    char *p = me;

    fprintf(fp,"Usage: %s: [options]\n", p);
    fprintf(fp,"Purpose: print current or convert between various time formats.\n");
    fprintf(fp,"         \"timestamp\" refers to YYYYMMDDhhmmss.\n");
    fprintf(fp,"%s -t              : print current UT as a timestamp\n", p);
    fprintf(fp,"%s -T <JD>         : convert given JD as a timestamp\n", p);
    fprintf(fp,"%s -u <t>          : convert UNIX time to a timestamp\n", p);
    fprintf(fp,"%s -U <timestamp>  : convert timestamp to UNIX time\n", p);
    fprintf(fp,"%s                 : print current UT as JD\n", p);
    fprintf(fp,"%s MMDDhhmmYYYY.ss : convert \"date\" format to JD\n", p);
    fprintf(fp,"%s <timestamp>     : convert timestamp to JD\n", p);
    fprintf(fp,"%s <JD>            : convert JD to \"DOW MMM DD HH:MM:SS UTC YYYY\"\n", p);

    exit(1);
}
