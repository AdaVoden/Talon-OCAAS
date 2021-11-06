#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "telstatshm.h"
#include "cliserv.h"
#include "strops.h"
#include "misc.h"

static void usage (char *p);

static char tcfn[] = "archive/config/telsched.cfg";
static char lgname[] = "LONGITUDE";

int
main(ac, av)
int ac;
char *av[];
{
	char *p = basenm(av[0]);
	TelStatShm *tp;
	char *lngenv;
	double l;
	char out[64];
	Now now;

	if (ac > 1)
	    usage (p);

	/* look around for time and longitude.
	 * first try LONGITUDE env, then shm, then telsched.cfg.
	*/
	if ((lngenv = getenv (lgname)) != 0) {
	    now.n_mjd = mjd_now();
	    now.n_lng = -atof (lngenv);	/* want +E */
	} else if (!open_telshm(&tp)) {
	    now = tp->now;
	} else if (!read1CfgEntry (0, tcfn, lgname, CFG_DBL, (void*)&l, 0)) {
	    now.n_mjd = mjd_now();
	    now.n_lng = -l;	/* want +E */
	} else 
	    usage (p);

	now_lst(&now, &l);
	range (&l, 24.0);

	fs_sexa (out, l, 2, 3600);
	printf ("%s\n", out);

	return (0);
}

static void
usage (char *p)
{
fprintf (stderr, "Usage: %s\n", p);
fprintf (stderr, "Purpose: compute Local Sidereal Time.\n");
fprintf (stderr, "Looks in the following places for longitude and time:\n");
fprintf (stderr, "  if LONGITUDE env is defined use it plus system time;\n");
fprintf (stderr, "  else if shared memory from telescoped is found use it for both;\n");
fprintf (stderr, "  else if telsched.cfg is found use it for longtude plus system.\n");

exit (1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: lst.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
