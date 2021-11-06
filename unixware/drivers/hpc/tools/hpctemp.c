#include <fcntl.h>
#include <sys/hpc.h>

char fn[] = "/dev/telescope/hpc";

main (ac, av)
int ac;
char *av[1];
{
	int fd;
	int t, temp;

	fd = open (fn, O_RDWR);
	if (fd < 0) {
	    perror (fn);
	    exit (1);
	}

	if (ioctl (fd, HPC_READ_TEMP, &t) < 0) {
	    perror (fn);
	    exit (1);
	}

	temp = (t & 0xff)/2 + 200;
	printf ("Raw=%d=0x%x; Temperature: %dK = %dC\n", t, t, temp, temp-273);
	close (fd);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: hpctemp.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
