#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void usage(void);

static char *prog;

int
main (ac, av)
int ac;
char *av[];
{
	char buf[64];
	int d, m;
	double s;
	double deg;
	int sign;
	char *signp;

	prog = av[0];

	if (ac == 4)
	    sprintf (buf, "%s:%s:%s", av[1], av[2], av[3]);
	else if (ac == 2)
	    strcpy (buf, av[1]);
	else
	    usage();;

	if ((signp = strchr (buf, '-')) != NULL) {
	    sign = -1;
	    *signp = ' ';
	} else
	    sign = 1;

	d = m = s = 0;
	if (sscanf (buf, "%d:%d:%lf", &d, &m, &s) < 1) {
	    fprintf (stderr, "Format should be d:m:s: %s\n", av[1]);
	    exit(1);
	}

	deg = sign*((s/60.0 + m)/60.0 + d);
	printf ("%g\n", M_PI*deg/180);

	return (0);
}

static void
usage()
{
	fprintf (stderr, "Usage: %s deg min sec or deg:min:sec\n", prog);
	exit (1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: degrad.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
