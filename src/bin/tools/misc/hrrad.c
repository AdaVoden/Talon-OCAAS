#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "P_.h"
#include "astro.h"

int
main (ac, av)
int ac;
char *av[];
{
	int d, m;
	double s;
	double deg;
	int sign;

	switch (ac) {
	case 4:
	    d = atoi (av[1]);
	    m = atoi (av[2]);
	    s = atof (av[3]);
	    break;
	case 2:
	    d = m = s;
	    if (sscanf (av[1], "%d:%d:%lf", &d, &m, &s) < 1) {
		fprintf (stderr, "Format should be d:m:s: %s\n", av[1]);
		exit(1);
	    }
	    break;
	default:
	    printf ("usage: deg min sec or deg:min:sec\n");
	    exit (1);
	}


	sign = 1;
	if (d < 0) {d = -d; sign = -1;}
	if (m < 0) {m = -m; sign = -1;}
	if (s < 0) {s = -s; sign = -1;}

	deg = sign*((s/60.0 + m)/60.0 + d);
	printf ("%g\n", hrrad(deg));

	return (0);
}
