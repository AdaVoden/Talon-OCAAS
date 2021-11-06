#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>

#include <sys/hpc.h>
#include "fits.h"

static char hpc_fn[] = "/dev/telescope/hpc";

int nohw;
int usepoll;

main (ac, av)
int ac;
char *av[];
{
	FImage fimage, *fip = &fimage;
	char errmsg[1024];
	int sx, sy, sw, sh;
	int bx, by;
	double dur;
	char *pix;
	char *p;
	int nbytes;
	HPCExpoParams h;
	char *str;
	int s;
	int fd;

	p = av[0];

	/* assume we open the shutter; -d will turn off to make a dark */
	h.shutter = 1;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while (c = *++str)
		switch (c) {
		case 'd':	/* take a dark, ie, don't open the shutter */
		    h.shutter = 0;
		    break;
		case 'p':	/* use poll(), not read, to wait */
		    usepoll++;
		    break;
		case 'h':	/* no hardware */
		    nohw++;
		    break;
		default:
		    usage(p);
		    break;
		}
	}

	if (ac != 3)
	    usage (p);

	s = sscanf (av[0], "%d+%dx%dx%d", &sx, &sy, &sw, &sh);
	if (s != 4)
	    usage(p);

	s = sscanf (av[1], "%dx%d", &bx, &by);
	if (s != 2)
	    usage(p);
	if (bx < 1 || by < 1) {
	    fprintf (stderr, "bx and by must be >= 1\n");
	    exit (1);
	}

	dur = atof (av[2]);

	h.sx = sx;
	h.sy = sy;
	h.sw = sw;
	h.sh = sh;
	h.bx = bx;
	h.by = by;
	h.duration = (int)(dur*1000.0);

	nbytes = sw/bx * sh/by * 2;	/* 2 bytes per pixel */
	pix = malloc (nbytes);
	if (!pix) {
	    fprintf (stderr, "Can not malloc %d bytes\n", nbytes);
	    exit (1);
	}

	if (nohw) 
	    fakeRaster (pix, sw/bx, sh/by);
	else {
	    int i;

	    if (usepoll)
		i = O_RDWR|O_NONBLOCK;
	    else
		i = O_RDWR;
	    fd = open (hpc_fn, i);
	    if (fd < 0) {
		perror (hpc_fn);
		exit(1);
	    }

	    s = ioctl (fd, HPC_SET_EXPO, &h);
	    if (s < 0) {
		perror ("HPC_SET_EXPO");
		exit (1);
	    }

	    if (usepoll) {
		struct pollfd pfd;
		int npfd = 1;

		/* do one read to start us off */
		(void) read (fd, 0, 1);

		fprintf (stderr, "Waiting for camera: ");

		do {
		    fprintf (stderr, ".");
		    pfd.fd = fd;
		    pfd.events = POLLRDNORM;
		    pfd.revents = 0;
		} while (poll (&pfd, npfd, 1000) == 0);

		fprintf (stderr, "\n");
	    }

	    s = read (fd, pix, nbytes);
	    if (s != nbytes) {
		perror ("read");
		exit (1);
	    }

	    close (fd);
	}

	/* make the FITS header */

	initFImage (fip);
	fip->bitpix = 16;
	fip->sw = sw/bx;
	fip->sh = sh/by;
	fip->sx = sx;
	fip->sy = sy;
	fip->bx = bx;
	fip->by = by;
	fip->dur = dur*1000;
	fip->image = pix;

	setSimpleFITSHeader (fip);

	if (writeFITS (1, fip, errmsg, 0) < 0)
	    printf ("%s\n", errmsg);
}

usage(p)
char *p;
{
	fprintf (stderr, "%s: [-phd] X+YxWxH BXxBY secs > filename\n", p);
	fprintf (stderr, "     -h: no hardware -- just fake an image.\n");
	fprintf (stderr, "     -p: use polling, not blocking read.\n");
	fprintf (stderr, "     -d: take a dark, ie, don't open shutter.\n");
	exit (1);
}

/* make a little picture of some sort.
 * pixels should be little-endian, 2-byte unsigned, first in upper left.
 */
fakeRaster (pix, w, h)
char *pix;
int w, h;
{
	unsigned int x, y, yy;
	unsigned short *p;
	int r = isqrt((w-1)*(w-1) + (h-1)*(h-1));

	p = (unsigned short *) pix;
	for (y = 0; y < h; y++) {
	    yy = y*y;
	    for (x = 0; x < w; x++)
		*p++ = isqrt (x*x + yy) * 65535 / r;
	}
}

isqrt (n)
int n;
{
	int i, s = 0, t;

	for (i = 15; i >= 0; i--) {
	    t = (s | (1 << i));
	    if (t * t <= n)
		s = t;
	}
	return (s);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: hpctest.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
