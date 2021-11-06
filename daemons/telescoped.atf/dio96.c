#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "P_.h"
#include "astro.h"
#include "telenv.h"
#include "dio.h"

int dio96_trace;

extern void die(void);
extern void tdlog (char *fmt, ...);

static int dio96_fd = -1;
static char dio96_fn[] = "dev/dio96";

static void dio96_showbits (unsigned char bits[12]);
static void dio96_open (void);

/* set 8255 control reg n to ctrl.
 */
void
dio96_ctrl (regn, ctrl)
int regn;
unsigned char ctrl;
{
	DIO_IOCTL dio;

	dio96_open();

	dio.n = regn;
	if (ioctl (dio96_fd, DIO_GET_REGS, &dio) < 0) {
	    perror ("setio -- get");
	    die();
	}

	dio.dio8255.ctrl = ctrl;
	if (ioctl (dio96_fd, DIO_SET_REGS, &dio) < 0) {
	    perror ("setio -- set");
	    die();
	}
}

void
dio96_getallbits (bits)
unsigned char bits[12];
{
	dio96_open();

	if (read (dio96_fd, bits, 12) < 0)
	    perror ("dio96 read");

	if (dio96_trace)
	    dio96_showbits (bits);
}

void
dio96_setbit (n)
int n;
{
	unsigned char b[12];

	if (dio96_trace)
	    tdlog ("dio96_setbit(%d)\n", n);

	dio96_open();

	read (dio96_fd, b, 12);
	b[n/8] |= 1 << (n%8);
	write (dio96_fd, b, 12);
}

void
dio96_clrbit (n)
int n;
{
	unsigned char b[12];

	if (dio96_trace)
	    tdlog ("dio96_clrbit(%d)\n", n);

	dio96_open();

	read (dio96_fd, b, 12);
	b[n/8] &= ~(1 << (n%8));
	write (dio96_fd, b, 12);
}

static void
dio96_showbits (bits)
unsigned char bits[12];
{
	int n = 12;	/* number of bytes */
	int i;

	/* show a chip counter */
	fprintf (stderr, "    ");
	for (i = n; --i >= 0; )
	    if ((i+1)%3) fprintf (stderr, "  ");
	    else         fprintf (stderr, "%d ", (i+1)/3-1);
	fprintf (stderr, "\n");

	fprintf (stderr, " r: ");
	for (i = n; --i >= 0; )
	    fprintf (stderr, "%02x", bits[i]);
	fprintf (stderr, "\n");
}

/* open the dio 96 driver.
 * if already open, do nothing quietly.
 * if can't, call exit().
 */
static void
dio96_open (void)
{
	if (dio96_fd >= 0)
	    return;

	dio96_fd = telopen (dio96_fn, O_RDWR);
	if (dio96_fd < 0) {
	    perror (dio96_fn);
	    exit (1);
	}

	if (dio96_trace)
	    tdlog ("dio96_open(%s): ok\n", dio96_fn);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: dio96.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
