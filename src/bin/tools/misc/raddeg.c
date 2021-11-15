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
	double rad;
	char out[64];

	if (ac != 2) {
	    printf ("usage: rads\n");
	    exit (1);
	}

	rad = atof (av[1]);
	fs_sexa (out, raddeg(rad), 3, 36000);

	printf ("%s\n", out);
	return (0);
}
