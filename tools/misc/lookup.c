/*
 * Lookup an object in the catalogs
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "catalogs.h"
#include "strops.h"
#include "configfile.h"
#include "misc.h"
#include "telenv.h"
#include "telstatshm.h"
#include "running.h"

TelStatShm * telstatshmp;

static void prAll (double ra, double dec, double e, double ha, double alt, double az);
int doLookup(char *obj, double ep);
static void initShm(char *progname);
static void usage(char *progname);

/*------------------*/

static int cflag;

int main (int ac, char *av[])
{
	char *progname = basenm(av[0]);
	char *str;
	int nobj;	
	double ep = EOD;
	
	initShm(progname);
		
	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str))
		switch (c) {
			case 'c':
				cflag++;
				break;
			case 'e':
				av++;
				ep = atod(*av);
				if(!ep) ep = EOD;
				else ep = (ep - 2000) * 365.25 + J2000;
				ac--;
				break;
		default:
		    usage(progname);
		    break;
		}
	}
	
	/* now there are ac remaining object names starting at av[0] */

	if (ac == 0) {
	    fprintf (stderr, "No object names\n");
	    usage (progname);
	}
	
	if(!cflag) {
		printf("%-10s  %-9s %-10s %-10s %-10s %-9s %-9s\n","Object","RA","DEC","ALT","AZ","HA","EPOCH");
		printf("==========================================================================\n");
	}

	nobj = 0;
	while (ac-- > 0) {
	    if (doLookup (*av++, ep) < 0)
		nobj++;
	}

	return 0;	
}

/* Lookup the object and print where it is */
int doLookup(char *obj, double ep)
{
	Now *np = &telstatshmp->now;
	Now hereNow;
	char buf[1024];
	int objl;
	int ret = -1;

	if(!obj) return -1;
	
	// Make a copy of the now object because we're about to touch it (and that's bad!)
	hereNow = *np;
	np = &hereNow;
	
	objl = strlen(obj);
	
	np->n_epoch = ep;
		
	/* see what we can do */
	if (objl > 0) {
	    /* lookup object */
	    char catdir[1024];
	    Obj o;

	    telfixpath (catdir, "archive/catalogs");
	    if (searchDirectory (catdir, obj, &o, buf) < 0) {
		/* unknown object */
		printf ("%s: %s\n", obj, buf);
	    } else {
		/* good object */
		double lst, ha;

		/* compute @ EOD */
		(void) obj_cir (np, &o);

		now_lst (np, &lst);
		ha = hrrad (lst) - o.s_ra;
		haRange (&ha);

		printf ("%-10s ", o.o_name);
		prAll (o.s_ra, o.s_dec, ep, ha, o.s_alt, o.s_az);
		ret = 0;
	    }	
	}	
	
	return ret;
	
}

static void
prAll (double ra, double dec, double e, double ha, double alt, double az)
{
	char buf[32];

	fs_sexa (buf, radhr(ra), 3, 3600);
	printf ("%-8s ", buf);
	fs_sexa (buf, raddeg(dec), 3, 3600);
	printf ("%-10s ",buf);
	fs_sexa (buf, raddeg(alt), 3, 3600);
	printf ("%-10s ",buf);
	fs_sexa (buf, raddeg(az), 3, 3600);
	printf ("%-10s ",buf);
	fs_sexa (buf, radhr(ha), 3, 3600);
	printf ("%-10s ",buf);
	if (e == EOD)
	    printf ("%-9s", "EOD");
	else {
	    double y;
	    mjd_year (e, &y);
	    printf ("%-9.1f", y);
	}
	printf("\n");
}

static void
initShm(char *progname)
{
	int shmid;
	long addr;

	shmid = shmget (TELSTATSHMKEY, sizeof(TelStatShm), 0);
	if (shmid < 0) {
	    perror ("shmget TELSTATSHMKEY");
	    unlock_running (progname, 0);
	    exit (1);
	}

	addr = (long) shmat (shmid, (void *)0, 0);
	if (addr == -1) {
	    perror ("shmat TELSTATSHMKEY");
	    unlock_running (progname, 0);
	    exit (1);
	}

	telstatshmp = (TelStatShm *) addr;
}

static void usage(char *progname)
{
	FILE *fp = stderr;

	fprintf(fp,"%s: usage: [options] objname [objname....]\n", progname);
	fprintf(fp,"   -c          Don't print column header\n");
	fprintf(fp,"   -e YYYY     epoch (i.e. 2000) Default is EOD.\n");
	exit (1);
}
	
