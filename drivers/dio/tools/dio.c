/* connect to the DIO dev entry in argv[1].
 * then be in a loop of reading/writing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "dio.h"
#include "telenv.h"

char prompt[] = ">>>  ";
char deffn[] = "dev/dio96";

#define	MAXCHIPS	4

main (ac, av)
int ac;
char *av[];
{
	int fd;
	char *fn;
	DIO_IOCTL dioctl[MAXCHIPS];
	unsigned char data[3*MAXCHIPS];
	char fullline[8*MAXCHIPS+1], line[8*MAXCHIPS+100];
	int nchips;
	int l, i;

	if (ac == 1)
	    fn = deffn;
	else if (ac == 2)
	    fn = av[1];
	else
	    usage();

	fd = telopen (fn, O_RDWR);
	if (fd < 0) {
	    perror (fn);
	    exit (1);
	}

	/* read just to learn number of chips */
	nchips = read (fd, data, sizeof(data));
	if (nchips < 0) {
	    perror ("read");
	    exit (1);
	}
	if (nchips%3) {
	    fprintf (stderr, "Read %d bytes\n", nchips);
	    exit(1);
	}
	nchips /= 3;
	if (nchips < 1 || nchips > MAXCHIPS) {
	    fprintf (stderr, "%d chips??\n", nchips);
	    exit(1);
	}

	while (1) {
	    int chip;
	    char *lp;

	    /* show an address counter */
	    printf ("Base+");
	    for (chip = nchips; --chip >= 0; )
		printf ("%8d", chip*4);
	    putchar ('\n');

	    /* show a register indicator */
	    printf (" Reg:");
	    for (chip = nchips; --chip >= 0; )
		printf ("ccCCBBAA");
	    putchar ('\n');

	    /* read and print each chip, saving in fullline[] too.
	     * N.B. read hi chip first, but displayed at front on line.
	     */
	    for (chip = nchips; --chip >= 0; ) {
		DIO_IOCTL *dp = &dioctl[chip];
		int start = (nchips-chip-1)*8;

		dp->n = chip;
		if (ioctl (fd, DIO_GET_REGS, (void *)dp) < 0) {
		    perror ("GET_REGS");
		    exit (1);
		}
		sprintf (fullline+start+0, "%02x", dp->dio8255.ctrl);
		sprintf (fullline+start+2, "%02x", dp->dio8255.portC);
		sprintf (fullline+start+4, "%02x", dp->dio8255.portB);
		sprintf (fullline+start+6, "%02x", dp->dio8255.portA);
	    }
	    printf ("Bits:%s\n", fullline);

	    /* prompt for and get a new line of input */
	    printf ("%s", prompt);
	    if (fgets (line, sizeof(line), stdin) == NULL)
		exit (0);
	    l = strlen (line);	/* remove trailing '\n' */
	    line[--l] = '\0';

	    /* if just type a newline, read again */
	    if (l == 0)
		continue;

	    /* give help if type a lone ? or a line that is too long */
	    if (line[0] == '?' || l > 8*nchips) {
		printf ("Type a new value of %d hex chars; use blanks to reuse a char.\n", 8*nchips);
		continue;
	    }

	    /* pad-right with blanks if line is short */
	    while (l < 8*nchips)
		line[l++] = ' ';

	    /* scan line, converting to hex and setting each chip */
	    printf ("Send:");
	    lp = line;
	    for (chip = nchips; --chip >= 0; ) {
		DIO_IOCTL *dp = &dioctl[chip];
		int start = (nchips-chip-1)*8;
		int err = 0;
		int reg;

		for (reg = 0; reg < 4; reg++) {
		    char c1 = *lp++;
		    char c0 = *lp++;
		    int  d1, d0;
		    int v;

		    if (c1 == ' ')
			c1 = fullline[start + 2*reg];
		    if (c0 == ' ')
			c0 = fullline[start + 2*reg + 1];
		    d1 = hex(c1);
		    d0 = hex(c0);

		    if (d1 < 0 || d0 < 0) {
			char bad = d1 < 0 ? c1 : c0;
			printf ("\nBad hex char: %c", bad);
			err = 1;
			break;
		    }

		    v = (d1 << 4) | d0;
		    printf ("%02x", v);

		    switch (reg) {
		    case 0: dp->dio8255.ctrl  = v; break;
		    case 1: dp->dio8255.portC = v; break;
		    case 2: dp->dio8255.portB = v; break;
		    case 3: dp->dio8255.portA = v; break;
		    }
		}

		if (err)
		    break;

		if (ioctl (fd, DIO_SET_REGS, (void *)dp) < 0) {
		    perror ("GET_SEGS");
		    exit (1);
		}
	    }

	    putchar ('\n');
	}
}

usage()
{
	printf ("usage: [dev/XXX]\n");
	exit (1);
}

int hex (c)
char c;
{
	int d;

	if ('0' <= c && c <= '9')
	    d = c - '0';
	else if ('a' <= c && c <= 'f')
	    d = c - 'a' + 10;
	else if ('A' <= c && c <= 'F')
	    d = c - 'A' + 10;
	else
	    d = -1;

	return (d);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: dio.c,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $"};
