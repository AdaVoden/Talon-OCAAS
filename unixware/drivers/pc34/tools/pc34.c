#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include <sys/pc34.h>

main (ac, av)
int ac;
char *av[];
{
	int pc34_fd;
	int nfds;
	int n;
	char *fn;
	char buf[128];
	fd_set rfds, wfds, xfds;
	struct timeval timeout;

	if (ac == 1) 
	    fn = "/dev/telescope/pc34";
	else if (ac == 2)
	    fn = av[1];
	else
	    usage();

	pc34_fd = open (fn, O_RDWR);
	if (pc34_fd < 0) {
	    perror (fn);
	    exit (1);
	}

	nfds = pc34_fd + 1;

	while (1) {
#ifdef SIMPLEIO
	    n = read (0, buf, sizeof(buf));
	    if (write (pc34_fd, buf, n) < 0)
		perror ("pc34 write");
	    n = read (pc34_fd, buf, sizeof(buf));
	    if (n < 0)
		perror ("pc34 read");
	    write (1, buf, n);
#else
	    FD_ZERO (&rfds);
	    FD_ZERO (&wfds);
	    FD_ZERO (&xfds);
	    FD_SET (0, &rfds);
	    FD_SET (pc34_fd, &rfds);

	    if (select (nfds, &rfds, &wfds, &xfds, NULL) < 0) {
		perror ("select");
		exit (1);
	    }

	    if (FD_ISSET (0, &rfds)) {
		n = read (0, buf, sizeof(buf));
		if (n <= 0)
		    break;
		if (buf[0] == 'd' && buf[1] == '\n') {
		    PC34_Regs regs;
		    if (ioctl (pc34_fd, PC34_GET_REGS, &regs) < 0)
			perror ("ioctl");
		    else
			printf ("Status=0x%x Ctrl=0x%x Done=0x%x\n", regs.stat,
							regs.ctrl, regs.done);
		    continue;
		}
		if (write (pc34_fd, buf, n) < 0) {
		    perror ("pc34 write");
		    exit (1);
		}
	    }
	    if (FD_ISSET (pc34_fd, &rfds)) {
		n = read (pc34_fd, buf, sizeof(buf));
		if (n < 0) {
		    perror ("pc34 read");
		    exit (1);
		}
		/* printf ("Read %d bytes from pc34_fd:\n", n); */
		(void) write (1, buf, n);
	    }
#endif
	}
}

usage()
{
	fprintf (stderr, "usage: [/dev/...]\n");
	exit (1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc34.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
