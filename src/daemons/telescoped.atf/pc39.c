/* code to handle basic communication with the OMS pc39 stepper motor
 * controller
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "pc39.h"
#include "teled.h"
#include "telenv.h"

int pc39_trace;

extern void die(void);
extern void tdlog (char *fmt, ...);

static void pc39_open(void);

static int pc39_fd = -1;		/* driver file des */
static char pc39_fn[] = "dev/pc39";	/* pathname, relative to TELHOME */

/* use this to send a command to the pc39 which requires no response.
 * return 0 if ok, else -1.
 * N.B. if want to send a command and listen for its response, use pc39_wr().
 * N.B. it is highly recommended to start all commands with the desired axis.
 */
int
pc39_w (char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	int l;

	va_start (ap, fmt);
	vsprintf (buf, fmt, ap);
	va_end (ap);

	l = strlen (buf);

	pc39_open();

	if (nohw > 0)
	    return(0);

	if (pc39_trace)
	    tdlog ("pc39_w: %s\n", buf);
	if (write (pc39_fd, buf, l) < 0) {
	    tdlog ("pc39_w(%s): %s", buf, strerror(errno));
	    return (-1);
	}

	return (0);
}

/* send one command/response pair to/from the pc39.
 * return n chars in rbuf if ok, else -1.
 * N.B. this should only be used when we indeed expect a response. if none
 *   is expected, use pc39_w().
 * N.B. it is highly recommended to start all commands with the desired axis
 *   in case other axes are in use.
 */
int
pc39_wr (char rbuf[], int rbuflen, char *fmt, ...)
{
	va_list ap;
	char wbuf[1024];
	PC39_OutIn oi;
	int n;

	va_start (ap, fmt);
	vsprintf (wbuf, fmt, ap);
	va_end (ap);

	pc39_open();

	if (nohw > 0) {
	    strcpy (rbuf, "0");
	    return(1);
	}

	oi.out = wbuf;
	oi.nout = strlen(wbuf);
	oi.in = rbuf;
	oi.nin = rbuflen;
	n = ioctl (pc39_fd, PC39_OUTIN, &oi);
	if (n < 0)
	    tdlog ("pc39_wr(%s): %s\n", wbuf, strerror(errno));
	else if (pc39_trace)
	    tdlog ("pc39_wr(): %s => %s\n", wbuf, rbuf);

	return (n);
}

/* do a full master reset of the board.
 */
int
pc39_reset()
{
	char buf[1024];
	int n;

	/* load a position then read back after hopefully as 0 to confirm.
	 * ignore return since we may be trying to get unstuck.
	 */
	if (pc39_trace)
	    tdlog ("Resetting board");
	(void) pc39_w ("ax lp1000 rs");

	/* manual says wait a bit */
	if (pc39_trace)
	    tdlog ("   sleeping");
	sleep(2);

	/* this one should be 0 for sure now */
	if (pc39_trace)
	    tdlog ("   checking reset");
	if ((n = pc39_wr (buf, sizeof(buf), "rp")) < 0) {
	    tdlog ("Reset confirmation 1 failed: %s", strerror(errno));
	    return (-1);
	}
	if (n == 0 || buf[0] != '0') {
	    tdlog ("Reset confirmation s failed");
	    return (-1);
	}

	/* phew! */
	return (0);
}

/* open the pc39 driver.
 * if already open, return quietly.
 * call exit() if can't (not die because it uses pc39!)
 * N.B. fake it if TELNOHW env is set.
 */
static void
pc39_open()
{
	static char me[] = "pc39_open";
	char buf[128];
	int n;

	if (nohw < 0)
	    nohw = getenv ("TELNOHW") ? 1 : 0;

	if (nohw > 0)
	    return;

	if (pc39_fd >= 0)
	    return;

	pc39_fd = telopen (pc39_fn, O_RDWR);
	if (pc39_fd < 0) {
	    tdlog ("%s: %s", pc39_fn, strerror(errno));
	    exit(1);
	}

	n = pc39_wr (buf, sizeof(buf), "wy");
	if (n <= 0) {
	    if (n < 0)
		tdlog ("%s(): WY failed: %s", me, strerror(errno));
	    else
		tdlog ("%s(): WY failed", me);
	    exit(1);
	} else
	    tdlog ("WY: %.*s", n, buf);

	if (pc39_trace)
	    tdlog ("%s(%s): ok\n", me, pc39_fn);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc39.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
