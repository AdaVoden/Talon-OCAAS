#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "telenv.h"
#include "pc39.h"

char dev[] = "dev/pc39";

int pc39;

void report();
void delay (int ms);

int
main (int ac, char *av[])
{
	pc39 = telopen (dev, O_RDWR);
	if (pc39 < 0) {
	    perror (dev);
	    exit(1);
	}

	setbuf (stdout, NULL);
	while (1) {
	    char buf[100];
	    report();
	    /*
	    delay (100);
	    */
	    if (fgets (buf, sizeof(buf), stdin) == NULL)
		break;
	}

	return (0);
}

void
delay (int ms)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = ms*1000;

	select (0, NULL, NULL, NULL, &tv);
}

void
report ()
{
	PC39_OutIn oi;
	char outb[128];
	char inb[128];
	int a, b;
	int n;

	sprintf (oi.out = outb, "ax re");
	oi.nout = strlen(oi.out);
	oi.in = inb;
	oi.nin = sizeof(inb);

	n = ioctl (pc39, PC39_OUTIN, &oi);
	if (n <= 0) {
	    perror ("ioctl ax re");
	    exit (2);
	}
	a = atoi(oi.in);

	sprintf (oi.out = outb, "ay re");
	oi.nout = strlen(oi.out);
	oi.in = inb;
	oi.nin = sizeof(inb);

	n = ioctl (pc39, PC39_OUTIN, &oi);
	if (n <= 0) {
	    perror ("ioctl ay re");
	    exit (3);
	}
	b = atoi (oi.in);

	printf ("%d %d\n", a, b);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: logencoders.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
