#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telenv.h"
#include "telstatshm.h"
#include "cliserv.h"
#include "pc39.h"

static void usage (char *p);
static void report(char axis);
static void delay (int ms);

static char dev[] = "dev/pc39";
static int lflag;
static int pc39;
static struct timeval tv0;
static TelStatShm *telstatshmp;

int
main (int ac, char *av[])
{
	char *pname = av[0];
	char axis;
	int ms;

	while ((--ac > 0) && ((*++av)[0] == '-')) {
	    char *s;
	    for (s = av[0]+1; *s != '\0'; s++)
		switch (*s) {
		case 'l':
		    lflag = 1;
		    break;
		default:
		    usage(pname);
		}
	}

	if (ac != 2)
	    usage (pname);
	ms = atoi (av[0]);
	axis = av[1][0];

	if (lflag) {
	    if (open_telshm (&telstatshmp) < 0) {
		perror ("telstatshm");
		exit (1);
	    }
	}

	pc39 = telopen (dev, O_RDWR);
	if (pc39 < 0) {
	    perror (dev);
	    exit(1);
	}

	setbuf (stdout, NULL);
	gettimeofday (&tv0, NULL);
	while (1) {
	    delay (ms);
	    report(axis);
	}
}

static void
usage (char *p)
{
    fprintf (stderr, "Usage: %s: [-l] <delay> <axis>\n", p);
    fprintf (stderr, "Purpose: log raw motor and encoder positions.\n");
    fprintf (stderr, "Optional arguments:\n");
    fprintf (stderr, " -l:    fourth column is 1 if scope is tracking\n");
    fprintf (stderr, "Required arguments:\n");
    fprintf (stderr, " delay: interval between polling, ms\n");
    fprintf (stderr, " axis:  one char to indicate pc39 axis\n");
    fprintf (stderr, "Output format is T M E [l]\n");
    fprintf (stderr, "  T:    elapsed time since program start, seconds\n");
    fprintf (stderr, "  M:    raw motor counts, from RP command\n");
    fprintf (stderr, "  E:    raw encoder counts, from RE command\n");
    fprintf (stderr, "  l:    optional: E if tracking else 0\n");

    exit(1);
}

static void
delay (int ms)
{
	struct timeval tv;

	if (ms == 0)
	    return;

	tv.tv_sec = ms/1000;
	tv.tv_usec = ms%1000;

	select (0, NULL, NULL, NULL, &tv);
}

static void
report (char axis)
{
	struct timeval tv;
	double dt;
	PC39_OutIn oi;
	char outb[128];
	char inb[128];
	int m, e;
	int n;

	gettimeofday (&tv, NULL);

	sprintf (oi.out = outb, "a%c rp", axis);
	oi.nout = strlen(oi.out);
	oi.in = inb;
	oi.nin = sizeof(inb);

	n = ioctl (pc39, PC39_OUTIN, &oi);
	if (n <= 0) {
	    perror ("ioctl rp");
	    exit (2);
	}
	m = atoi(oi.in);

	sprintf (oi.out = outb, "a%c re", axis);
	oi.nout = strlen(oi.out);
	oi.in = inb;
	oi.nin = sizeof(inb);

	n = ioctl (pc39, PC39_OUTIN, &oi);
	if (n <= 0) {
	    perror ("ioctl re");
	    exit (3);
	}
	e = atoi (oi.in);

	dt = (tv.tv_sec - tv0.tv_sec) + (tv.tv_usec - tv0.tv_usec)/1e6;
	printf ("%7.2f %6d %6d", dt, m, e);

	if (telstatshmp)
	    printf (" %6d", telstatshmp->telstate == TS_TRACKING ? e : 0);
	printf ("\n");
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: logaxis.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
