/* open the pc39 and send each command in argv[] and print atomic responses.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "telenv.h"
#include "pc39.h"

char dev[] = "dev/pc39";

int verbose;

int
main (int ac, char *av[])
{
	int pc39;
	PC39_OutIn oi;
	char inb[128];
	int n;

	pc39 = telopen (dev, O_RDWR);
	if (pc39 < 0) {
	    perror (dev);
	    exit(1);
	}
	
	--ac;
	av++;

	if (ac > 0 && strcmp (av[0], "-v") == 0) {
	    verbose++;
	    --ac;
	    av++;
	}

	for (; ac-- > 0; av++) {
	    oi.out = av[0];
	    oi.nout = strlen(av[0]);
	    oi.in = inb;
	    oi.nin = sizeof(inb);

	    n = ioctl (pc39, PC39_OUTIN, &oi);
	    if (n < 0)
		perror ("ioctl");
	    else {
		if (verbose)
		    fprintf (stderr, "%s -> %.*s\n", oi.out, n, oi.in);
		if (n > 0)
		    printf ("%.*s\n", n, oi.in);
	    }
	}

	return(0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc39cmd.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
