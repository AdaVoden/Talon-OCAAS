/* set the parallel port output port to our arg.
 * N.B. must be suid root
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>	/* for libc5 */
#include <sys/io.h>	/* for glibc */

#define	LPBASE	0x378

static void usage (void);

static int flag0;
static int flag1;
static int flagt;
static int flagr;
static int flagv;
static char *prog;

int
main (int ac, char *av[])
{
	unsigned char v = 0;

	prog = av[0];

        for (av++; --ac > 0 && **av == '-'; av++) {
            char c, *str = *av;
            while ((c = *++str)) {
                switch (c) {
                case '0':
		    if (ac < 2)
			usage();
		    v = (unsigned char) strtol (*++av, NULL, 0);
		    ac--;
                    flag0 = 1;
                    break;
                case '1':
		    if (ac < 2)
			usage();
		    v = (unsigned char) strtol (*++av, NULL, 0);
		    ac--;
                    flag1 = 1;
                    break;
                case 't':
		    if (ac < 2)
			usage();
		    v = (unsigned char) strtol (*++av, NULL, 0);
		    ac--;
                    flagt = 1;
                    break;
                case 'r':
                    flagr = 1;
                    break;
                case 'v':
                    flagv = 1;
                    break;
		default:
		    usage ();
		    break;
		}
	    }
	}

	/* no more args */
	if (ac > 0)
	    usage();

	/* map -- need root perm */
	if (ioperm (LPBASE, 3, ~0) < 0) {
	    perror("ioperm");
	    exit (1);
	}

	/* do write, if any */
	if (flag0 + flag1 + flagt > 0) {
	    unsigned char lpnow, lpnew = 0;

	    if (flag0 + flag1 + flagt > 1)
		usage ();		/* just 1 */

	    lpnow = inb (LPBASE);
	    if (flag0)
		lpnew = lpnow & ~v;
	    if (flag1)
		lpnew = lpnow |  v;
	    if (flagt)
		lpnew = lpnow ^  v;
	    outb(lpnew, LPBASE);
	    if (flagv)
		printf ("v = 0x%02x 0x%02x <- inb(0x%02x), outb(0x%02x, 0x%02x)\n",
					    v, lpnow, LPBASE, lpnew, LPBASE);
	}

	/* do read, if desired */
	if (flagr)
	    printf ("0x%02x\n", inb (LPBASE));

	return (0);
}

static void
usage ()
{
	fprintf(stderr,"Usage: %s [options]\n", prog);
	fprintf(stderr,"Purpose: manipulate and/or read parallel port data bits \n");
	fprintf(stderr,"Options:\n");
	fprintf(stderr," -0 v: turn off LP data bits that are 1 in <v>\n");
	fprintf(stderr," -1 v: turn on  LP data bits that are 1 in <v>\n");
	fprintf(stderr," -t v: toggle   LP data bits that are 1 in <v>\n");
	fprintf(stderr," -r  : read and report current (or resulting) LP data bits\n");
	fprintf(stderr," -v  : print outb actually written\n");
	exit (1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: lpio.c,v $ $Date: 2003/04/15 20:48:38 $ $Revision: 1.1.1.1 $ $Name:  $"};
