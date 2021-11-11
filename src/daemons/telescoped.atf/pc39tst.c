#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

extern pc39_trace;

#define	N	1000

void oneTest (char *cmd, char *old);

int check;

main (int ac, char *av[])
{
	static char lxp[10], lyp[10], lzp[10], lup[10], lvp[10];
	static char lxv[10], lyv[10], lzv[10], luv[10], lvv[10];
	static char lxa[10], lya[10], lza[10], lua[10], lva[10];
	int i;

	/* pc39_trace = 1; */

	check = ac > 1;

	for (i = 0; i < N; i++) {
	    oneTest ("ax re", lxp);
	    oneTest ("ax rv", lxv);
	    /* oneTest ("ax qa", lxa); */
	    oneTest ("ay re", lyp);
	    oneTest ("ay rv", lyv);
	    /* oneTest ("ay qa", lya); */
	    oneTest ("az rp", lzp);
	    oneTest ("az rv", lzv);
	    /* oneTest ("az qa", lza); */
	    oneTest ("av rp", lvp);
	    oneTest ("av rv", lvv);
	    /* oneTest ("av qa", lva); */
	    oneTest ("au rp", lup);
	    oneTest ("au rv", luv);
	    /* oneTest ("au qa", lua); */
	}
}

void
oneTest (char *cmd, char *old)
{
	char back[32];

	if (pc39_wr (back, sizeof(back), cmd) < 0) {
	    perror (cmd);
	    exit(1);
	}

	if (check && strcmp (old, back)) {
	    printf ("%s => %s\n", cmd, back);
	    strcpy (old, back);
	}
}



/* print a message to stderr */
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
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc39tst.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
