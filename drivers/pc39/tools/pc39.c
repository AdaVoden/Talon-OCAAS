/* simple program to open the pc39 and interact with it on stdin/out.
 * for use with scripts, all chars from SCOMMENT to ECOMMENT are ignored.
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "telenv.h"
#include "running.h"
#include "strops.h"
#include "pc39.h"


static void usage (char *p);
static void send_command (char buf[], int nbuf);

#define SCOMMENT	'#'
#define	ECOMMENT	'\n'

static char dev[] = "dev/pc39";
static int pc39;

/* commands which have responses */
static char *rspcmds[] = {
    /* status commands */
    "wy",
    "rp",
    "rq",
    "ra",
    "ri",
    "qa",
    "qi",
    "rc",
    "rv",
    "ru",

    /* encoder commands */
    "rl",
    "ea",
    "re",
};
#define	NRSPCMDS	(sizeof(rspcmds)/sizeof(rspcmds[0]))

static int qflag;

int
main (int ac, char *av[])
{
	char *prog = av[0];
	enum {IN_CMD, IN_COMMENT} state = IN_CMD;

	/* crack arguments */
	for (av++; --ac > 0 && *(*av) == '-'; av++) {
	    char c;
	    while ((c = *++(*av)) != '\0') {
		switch (c) {
		case 'q':
		    qflag++;
		    break;
		default:
		    usage (prog);
		    break;
		}
	    }
	}

	if (!qflag && testlock_running("telescoped") == 0)
	    printf ("CAUTION: telescoped is running -- prefix all commands with axis\n");

	pc39 = telopen (dev, O_RDWR);
	if (pc39 < 0) {
	    perror (dev);
	    exit(1);
	}

	for (;;) {
	    fd_set rfds;
	    char buf[256];
	    int nr;

	    FD_ZERO (&rfds);
	    FD_SET (0, &rfds);
	    FD_SET (pc39, &rfds);
	    if (select (pc39+1, &rfds, NULL, NULL, NULL) < 0) {
		perror ("select");
		continue;
	    }

	    if (FD_ISSET (0, &rfds)) {
		char outbuf[sizeof(buf)];
		int nob;
		int i;

		/* read and scrub out comments.
		 * N.B. state must be global in case a line splits.
		 */
		if ((nr = read (0, buf, sizeof(buf))) <= 0)
		    break;
		for (nob = i = 0; i < nr; i++) {
		    char c = buf[i];
		    switch (state) {
		    case IN_CMD:
			if (c == SCOMMENT)
			    state = IN_COMMENT;
			else
			    outbuf[nob++] = c;
			break;
		    case IN_COMMENT:
			if (c == ECOMMENT)
			    state = IN_CMD;
			break;
		    }
		}

		/* send buf to pc39, and get response if any */
		send_command (outbuf, nob);
	    }

	    if (FD_ISSET (pc39, &rfds)) {
		nr = read (pc39, buf, sizeof(buf));
		if (nr < 0)
		    perror ("pc39 read");
		else if (nr == 0)
		    printf ("read returned 0\n");
		else {
		    printf ("%.*s\n", nr, buf);
		    fflush (stdout);
		}
	    }
	}

	return(0);
}

static void
usage (char *p)
{
	fprintf (stderr, "Usage: %s [options]\n", basenm(p));
	fprintf (stderr, "Purpose: connect stdin/out directly to OMS PC39\n");
	fprintf (stderr, " -q: quiet; do not warn if PC39 already in use\n");

	exit (1);
}

static void
send_command (char buf[], int nbuf)
{
	int i;

	/* send with pc39_wr if expect a response */
	for (i = 0; i < NRSPCMDS; i++) {
	    if (strstr (buf, rspcmds[i])) {
		char back[1024];
		PC39_OutIn oi;
		int n;

		oi.out = buf;
		oi.nout = nbuf;
		oi.in = back;
		oi.nin = sizeof(back);
		n = ioctl (pc39, PC39_OUTIN, &oi);
		if (n < 0)
		    perror ("ioctl");
		else if (n == 0)
		    printf ("ioctl returned 0\n");
		else if (n > 0)
		    printf ("%.*s\n", n, oi.in);
		return;
	    }
	}

	/* else just cast to the wind */
	if (nbuf > 0 && write (pc39, buf, nbuf) != nbuf)
	    perror ("pc39 write");
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc39.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
