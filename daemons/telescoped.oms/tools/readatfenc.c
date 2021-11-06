/* read and print the raw encoders.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include "dio.h"

static void usage (char *p);
static void tel_read_enc (int *ha, int *dec);

int
main (int ac, char *av[])
{
	char *p = av[0];
	int loops = 1;
	int lastha, lastdec;

	for (av++; --ac > 0 && **av == '-'; av++) {
	    char c, *str = *av;
	    while ((c = *++str) != '\0') {
		switch (c) {
		case 't':
		    dio96_trace++;
		    break;
		case 'l':
		    if (ac < 2)
			usage (p);
		    loops = atoi (*++av);
		    ac--;
		    break;
		default:
		    usage (p);
		}
	    }
	}

	lastha = 10000000;
	lastdec = 10000000;
	while (1) {
	    int ha, dec;
	    tel_read_enc(&ha, &dec);
	    if (ha != lastha || dec != lastdec) {
		printf ("ha=%5d dec=%5d\n", ha, dec);
		lastha = ha;
		lastdec = dec;
	    }
	    if (--loops <= 0)
		break;
	    sleep (1);
	}

	return (0);
}

static void
usage (char *p)
{
	fprintf (stderr, "Usage: %s [options]\n", p);
	fprintf (stderr, "Purpose: read raw ATF DIO96 telescope encoders.\n");
	fprintf (stderr, "Print both whenever either changes value.\n");
	fprintf (stderr, "Options:\n");
	fprintf (stderr, " -t  : trace:\n");
	fprintf (stderr, " -l n: loop n times, default is 1:\n");

	exit (1);
}

static void
tel_read_enc (int *haenc, int *decenc)
{
	unsigned char bits[12];

	/* set up 8255s as input for ha/dec encoders, output for TG bit */
	dio96_ctrl (0, 0x9b);    /* all lines input on 1st chip */
	dio96_ctrl (1, 0x93);    /* all lines input on 2nd chip but C-hi */

	/* strobe the TG encoders, delay, read */
	dio96_clrbit (44);	/* clear the TG bit */
	dio96_setbit (44);	/* set the TG bit */

	(void) usleep (10000);	/* delay, in us */
	dio96_getallbits (bits);/* read all encoders */

	*haenc  = ((unsigned)bits[1]<<8) | (unsigned)bits[0];
	*decenc = ((unsigned)bits[4]<<8) | (unsigned)bits[3];

}

void
die()
{
	exit (1);
}

void
tdlog (char *fmt, ...)
{
	char datebuf[128], msgbuf[1024];
	va_list ap;
	time_t t;
	struct tm *tmp;

	time (&t);
	tmp = gmtime (&t);
	sprintf (datebuf, "%02d/%02d/%02d %02d:%02d:%02d: ", tmp->tm_mon+1,
	    tmp->tm_mday, tmp->tm_year, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

	va_start (ap, fmt);

	fputs (datebuf, stderr);
	vsprintf (msgbuf, fmt, ap);
	fputs (msgbuf, stderr);
	if (msgbuf[strlen(msgbuf)-1] != '\n')
	    fputs ("\n", stderr);

	va_end (ap);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: readatfenc.c,v $ $Date: 2003/04/15 20:48:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
