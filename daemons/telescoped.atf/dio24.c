#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "P_.h"
#include "astro.h"
#include "telenv.h"
#include "dio.h"

int dio24_trace;

extern void die(void);
extern void tdlog (char *fmt, ...);

static int dio24_fd = -1;
static char dio24_fn[] = "dev/dio24";

static void dio24_showbits (unsigned char bits[3]);
static void dio24_open (void);

/* set 8255 control reg n to ctrl.
 */
void
dio24_ctrl (regn, ctrl)
int regn;
unsigned char ctrl;
{
	DIO_IOCTL dio;

	dio24_open();

	dio.n = regn;
	if (ioctl (dio24_fd, DIO_GET_REGS, &dio) < 0) {
	    perror ("setio -- get");
	    die();
	}

	dio.dio8255.ctrl = ctrl;
	if (ioctl (dio24_fd, DIO_SET_REGS, &dio) < 0) {
	    perror ("setio -- set");
	    die();
	}
}

void
dio24_getallbits (bits)
unsigned char bits[3];
{
	dio24_open();

	if (read (dio24_fd, bits, 3) < 0)
	    perror ("dio24 read");

	if (dio24_trace)
	    dio24_showbits (bits);
}

void
dio24_setbit (n)
int n;
{
	unsigned char b[3];

	if (dio24_trace)
	    tdlog ("dio24_setbit(%d)\n", n);

	dio24_open();

	read (dio24_fd, b, 3);
	b[n/8] |= 1 << (n%8);
	write (dio24_fd, b, 3);
}

void
dio24_clrbit (n)
int n;
{
	unsigned char b[3];

	if (dio24_trace)
	    tdlog ("dio24_clrbit(%d)\n", n);

	dio24_open();

	read (dio24_fd, b, 3);
	b[n/8] &= ~(1 << (n%8));
	write (dio24_fd, b, 3);
}

static void
dio24_showbits (bits)
unsigned char bits[3];
{
	int n = 3;	/* number of bytes */
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

/* open the dio 24 driver.
 * if already open, do nothing quietly.
 * if can't, call exit().
 */
static void
dio24_open (void)
{
	if (dio24_fd >= 0)
	    return;

	dio24_fd = telopen (dio24_fn, O_RDWR);
	if (dio24_fd < 0) {
	    perror (dio24_fn);
	    exit (1);
	}

	if (dio24_trace)
	    tdlog ("dio24_open(%s): ok\n", dio24_fn);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: dio24.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
