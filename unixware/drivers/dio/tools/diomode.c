#include <stdio.h>
#include <sys/dio.h>

char deffn[] = "/dev/telescope/dio96";

main (ac, av)
int ac;
char *av[];
{
	int fd;
	char *fn;
	char rw;
	int regn;
	int s;
	DIO_IOCTL dio;

	if (ac < 4)
	    usage();

	fn = av[1];
	fd = open (fn, 2);
	if (fd < 0) {
	    fn = deffn;
	    fd = open (fn, 2);
	    if (fd < 0) {
		perror (fn);
		exit (1);
	    }
	}

	rw = av[2][0];
	if (rw != 'r' && rw != 'w')
	    usage();
	regn = atoi (av[3]);
	dio.n = regn;

	if (rw == 'r') {
	    if (ac != 4)
		usage();
	    s = ioctl (fd, DIO_GET_REGS, &dio);
	    if (s < 0) {
		perror ("ioctl");
		exit (1);
	    }
	    printf ("8255 %d: C=%02x B=%02x A=%02x Ctrl=%02x\n", dio.n,
		dio.dio8255.portC, dio.dio8255.portB, dio.dio8255.portA,
							dio.dio8255.ctrl);
	} else {
	    int i;

	    if (ac != 8)
		usage();
	    sscanf (av[4], "%x", &i);
	    dio.dio8255.portA = i;
	    sscanf (av[5], "%x", &i);
	    dio.dio8255.portB = i;
	    sscanf (av[6], "%x", &i);
	    dio.dio8255.portC = i;
	    sscanf (av[7], "%x", &i);
	    dio.dio8255.ctrl  = i;
	    s = ioctl (fd, DIO_SET_REGS, &dio);
	    if (s < 0) {
		perror ("ioctl");
		exit (1);
	    }
	    ioctl (fd, DIO_GET_REGS, &dio);
	    printf ("8255 %d: C=%02x B=%02x A=%02x Ctrl=%02x\n", dio.n,
		dio.dio8255.portC, dio.dio8255.portB, dio.dio8255.portA,
							dio.dio8255.ctrl);
	}
}

usage()
{
	printf ("usage: /dev/telesope/dioXXX {rw} n [A B C CTRL]\n");
	exit (1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: diomode.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
