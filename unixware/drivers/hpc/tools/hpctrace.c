#include <fcntl.h>
#include <sys/hpc.h>

char fn[] = "/dev/telescope/hpc";

main (ac, av)
int ac;
char *av[1];
{
	int fd;
	int on;

	if (ac != 2) {
	    printf ("usage: <trace_level>\n");
	    exit (1);
	}

	on = atoi (av[1]);

	fd = open (fn, O_RDWR);
	if (fd < 0) {
	    perror (fn);
	    exit (1);
	}

	printf ("Turning tracing %s\n", on ? "on" : "off");

	if (ioctl (fd, HPC_TRACE, on) < 0) {
	    perror (fn);
	    exit (1);
	}

	close (fd);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: hpctrace.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
