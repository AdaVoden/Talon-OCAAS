/* run a command periodically and display it's standard output in screen mode.
 */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define	DEFSECS		5	/* default secs between runs */

#define BUFSZ		1024
#define NCOLS		80
#define NROWS		24
#define SUPERUSER	0

#define match(s1,s2)	(strcmp(s1, s2) == 0)
#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))

typedef int Bool;
typedef int Status;

Bool done = FALSE;
Bool initialized = FALSE;

Bool getargs (int ac, char **av, int *seconds, char *cmd);
void on_int (int);
Status runcmd (char *cmd);
void start (void);
void stop (void);
Bool suser (void);
void usage (void);
void winsize(int *nrows, int *ncols);

char *progname;
int eflag;

int
main (int ac, char **av)
{
	char cmd[256];
	int seconds;
	Status e;

	signal (SIGINT, on_int);
	progname = av[0];
	if (!getargs (ac, av, &seconds, cmd))
	    usage ();
	if (eflag)
	    dup2 (open ("/dev/null", 0), 2);
	start ();
	while (!done)
	{
	    if ((e = runcmd (cmd)) != 0)
	    {
		fprintf (stderr, "(%s): %d\n", cmd, e);
		return (e);
	    }
	    if (seconds > 0)
		sleep (seconds);
	}
	return (0);
}

/* crack the argument list
 */
Bool
getargs (int ac, char **av, int *seconds, char *cmd)
{
	char *str;

	/* set up default values */
	*cmd = 0;
	*seconds = DEFSECS;

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str))
		switch (c) {
		case 's':
		    if (ac < 2)
			usage ();
		    sscanf (*++av, "%d", seconds);
		    ac--;
		    break;
		case 'e':
		    eflag++;
		    break;
		default:
		    usage ();
		    break;
		}
	}

	/* build up the command to run */
	while (ac-- > 0)
	{
	    strcat (cmd, *av++);
	    strcat (cmd, " ");
	}

	/* sanity checks */
	if (*cmd == 0)
	    return (FALSE);
	if (*seconds < 0)
	    return (FALSE);
	if (*seconds == 0 && !suser ())
	    *seconds = 1;

	/* ok */
	return (TRUE);
}

/* interrupt handler
 */
void
on_int (int i)
{
	signal (SIGINT, on_int);
	stop ();
	exit (0);
}

/* run the command
 */
Status
runcmd (char *cmd)
{
	FILE *f;
	char buf[BUFSZ];
	register int lineno;
	register int len;
	int thisnlines;
	static int lastnlines = 0;
	int ncols;
	int nrows;

	winsize(&nrows, &ncols);

	if ((f = popen (cmd, "r")) == (FILE *)0)
	    return (errno);
	move (0, 0);
	for (lineno = 0; fgets (buf, BUFSZ, f) != (char *)0; lineno++)
	{
	    if (lineno < nrows)
	    {
		if ((len = strlen (buf)) >= ncols)
		{
		    *(buf + ncols - 2) = '\n';
		    *(buf + ncols - 1) = 0;
		}
		mvaddstr (lineno, 0, buf);
		refresh ();
	    }
	}
	thisnlines = --lineno;
	lineno = min (lineno, nrows);
	while (lineno++ < lastnlines)
	    mvaddstr (lineno, 0, "\n");
	move (0, 0);
	refresh ();
	lastnlines = thisnlines;
	if (pclose (f) < 0)
	    return (errno);
	return (0);
}

/* initialize curses
 */
void
start (void)
{
	initscr ();
	savetty ();
	clear ();
	refresh ();
	initialized = TRUE;
}

/* shutdown curses
 */
void
stop (void)
{
	if (initialized)
	{
	    initialized = FALSE;
	    move (0, 0);
	    clear ();
	    refresh ();
	    resetty ();
	    endwin ();
	}
}

/* tell whether or not super-user
 */
Bool
suser (void)
{
	if (geteuid () == SUPERUSER)
	    return (TRUE);
	else
	    return (FALSE);
}

/* display the usage message
 */
void
usage ()
{
	fprintf (stderr, "%s: [options] command\n", progname);
	fprintf (stderr, "Purpose: run command repeatedly on the screen.\n");
	fprintf (stderr, "  -s n: repeat every n seconds; default is %d\n",
								    DEFSECS);
	fprintf (stderr, "  -e  : discard the stderr stream\n");

	exit (1);
}

/* get the window size
 */
void winsize(int *nrows, int *ncols)
{
    struct winsize ws;
    int fd = 0;

    *nrows = NROWS;
    *ncols = NCOLS;

    if (!isatty(0) && (fd = open("/dev/tty", O_RDONLY)) == -1)
	return;

    if (ioctl(fd, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0 && ws.ws_row > 0)
    {
	*nrows = ws.ws_row;
	*ncols = ws.ws_col;
    }
    close(fd);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: screen.c,v $ $Date: 2003/04/15 20:48:39 $ $Revision: 1.1.1.1 $ $Name:  $"};
