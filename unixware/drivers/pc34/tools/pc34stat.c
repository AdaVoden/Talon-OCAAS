#include <sys/pc34.h>

main()
{
	int fd;
	char *fn;
	PC34_Regs regs;

	fn = "/dev/telescope/pc34";
	fd = open (fn, 0);
	if (fd < 0) {
	    perror (fn);
	    exit (1);
	}
	if (ioctl (fd, PC34_GET_REGS, &regs) < 0) {
	    perror ("ioctl");
	    exit (2);
	}

	printf ("done = 0x%x\n", regs.done);
	printf ("stat = 0x%x\n", regs.stat);
	printf ("ctrl = 0x%x\n", regs.ctrl);

	if (close (fd) < 0) {
	    perror ("close");
	    exit (3);
	}

	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc34stat.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
