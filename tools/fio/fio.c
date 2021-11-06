/* simple program to connect to a fifo pair in archive/comm */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "strops.h"
#include "telstatshm.h"
#include "running.h"
#include "cliserv.h"

static void usage (char *p);
static void beep (void);

static int bflag;
static int wflag;
static int mflag;
static int qflag;
static int rflag;
static int sflag;
static int tflag;

int
main (int ac, char *av[])
{
	char *progname = av[0];
	int stdineof = 0;
	char buf[1024];
	char *whom;
	int fd[2];

	/* instant reports */
	setbuf (stdout, NULL);

	/* we can not share reading the fifos */
	if (testlock_running("xobs") == 0) {
	    fprintf (stderr, "xobs is running -- only writing to fifos\n");
	    wflag = 1;
	}
	if (testlock_running("telrun") == 0) {
	    fprintf (stderr, "telrun is running -- only writing to fifos\n");
	    wflag = 1;
	}

	while ((--ac > 0) && ((*++av)[0] == '-')) {
	    char *s;
	    for (s = av[0]+1; *s != '\0'; s++)
		switch (*s) {
		case 'b':
		    bflag++;
		    break;
		case 'm':
		    mflag++;
		    break;
		case 'q':
		    qflag++;
		    break;
		case 'r':
		    if (ac < 2)
			usage(progname);
		    rflag = atoi (*++av);
		    ac--;
		    break;
		case 's':
		    sflag++;
		    break;
		case 't':
		    if (ac < 2)
			usage(progname);
		    tflag = atoi (*++av);
		    ac--;
		    break;
		case 'w':
		    wflag++;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* sanity checks */
	if (ac != 1)
	    usage (progname);
	whom = av[0];
	if (mflag && wflag) {
	    fprintf (stderr, "%s: No use having both -m and -w\n", whom);
	    usage (progname);
	}

	/* open connection, retrying if desired */
	for (; rflag >= 0; rflag--) {
	    if (cli_conn(whom, fd, buf) == 0)
		break;
	    if (rflag > 0)
		sleep (1);
	}
	if (rflag < 0) {
	    fprintf (stderr, "%s: %s\n", whom, buf);
	    exit (100);
	}

	while (1) {
	    struct timeval tv, *tvp = &tv;
	    fd_set rdset;
	    int maxfdp1;
	    int n;

	    FD_ZERO (&rdset);
	    maxfdp1 = 0;
	    if (!mflag && !stdineof) {
		FD_SET (0, &rdset);
		maxfdp1 = 1;
	    }
	    if (!wflag) {
		FD_SET (fd[0], &rdset);
		maxfdp1 = fd[0]+1;
	    }

	    if (tflag) {
		tvp->tv_sec = tflag;
		tvp->tv_usec = 0;
	    } else
		tvp = NULL;

	    n = select (maxfdp1, &rdset, NULL, NULL, tvp);
	    if (n < 0) {
		fprintf (stderr, "%s: select: %s\n", whom, strerror(errno));
		exit (101);
	    }
	    if (n == 0) {
		if (!qflag)
		    fprintf (stderr, "%s: timeout\n", whom);
		exit (102);
	    }

	    if (!mflag && FD_ISSET (0, &rdset)) {
		n = read (0, buf, sizeof(buf));
		if (n < 0) {
		    fprintf (stderr, "%s: stdin: %s\n", whom, strerror(errno));
		    exit (103);
		} else if (n == 0) {
		    if (!sflag)
			exit (105);
		    stdineof = 1;
		} else {
		    buf[n] = '\0';
		    if (cli_write (fd, buf, buf) < 0) {
			fprintf (stderr, "%s: write: %s\n", whom, buf);
			exit (108);
		    }
		}
	    }

	    if (!wflag && FD_ISSET (fd[0], &rdset)) {
		int code;

		if (cli_read (fd, &code, buf, sizeof(buf)) < 0) {
		    fprintf (stderr, "%s: read: %s\n", whom, buf);
		    exit (104);
		} else {
		    if (!qflag)
			printf ("%10s: %3d: %s\n", whom, code, buf);
		    if (bflag && code <= 0)
			beep();
		    if (sflag && code <= 0)
			exit(abs(code));
		}
	    }
	}

	/* can't happen */
	return (106);
}

/* ring the bell */
static void
beep ()
{
	write (1, "\a", 1);
}

static void
usage(char *progname)
{
#define	FPE	fprintf (stderr, 

FPE "%s: [options] <fifo>\n", basenm(progname));
FPE "Purpose: attach to the given fifo in $TELHOME/comm.\n");
FPE "  connect our stdin to fifo.in and fifo.out to our stdout.\n");
FPE "Options:\n");
FPE " -b  : beep when receive code <= 0\n");
FPE " -m  : read-only (do not connect to fifo.in)\n");
FPE " -q  : do not echo fifo.out (just read and discard silently)\n");
FPE " -r n: retry open up to n secs (default tries only once)\n");
FPE " -s  : exit if code <= 0 (default is to exit on eof from stdin)\n");
FPE " -t n: exit if no fifo.out traffic for n seconds (default waits forever)\n");
FPE " -w  : write-only (do not connect to fifo.out)\n");
FPE "\n");
FPE "exit values:\n");
FPE " 100 : could not open fifo\n");
FPE " 101 : select() error\n");
FPE " 102 : fifo.out timed out\n");
FPE " 103 : error reading stdin\n");
FPE " 104 : error reading fifo.out\n");
FPE " 105 : eof from stdin and no -s");
FPE " 106 : can't happen\n");
FPE " 107 : usage error\n");
FPE " 108 : error writing fifo.in\n");
FPE " ??? : any other exit value is abs(code) from fifo.out\n");

exit (107);
}
