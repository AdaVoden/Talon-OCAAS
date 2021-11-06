/* connect to the given /dev entry in argv[1].
 * then be in a loop of reading/writing.
 */

#include <stdio.h>
#include <sys/dio.h>

char prompt[] = ">>> ";
char deffn[] = "/dev/telescope/dio96";

main (ac, av)
int ac;
char *av[];
{
	int fd;
	char *fn;
	unsigned char data[32];
	char line[128];
	int l, i, n;

	if (ac == 1)
	    fn = deffn;
	else if (ac == 2)
	    fn = av[1];
	else
	    usage();

	fd = open (fn, 2);
	if (fd < 0) {
	    perror (fn);
	    exit (1);
	}

	while (1) {
	    int error = 0;

	    /* read and a print the current bits */
	    n = read (fd, data, sizeof(data));
	    if (n < 0) {
		perror ("read");
		exit (1);
	    }

	    /* show a chip counter if there is more than one */
	    if (n > 3) {
		printf ("    ");
		for (i = n; --i >= 0; )
		    if ((i+1)%3) printf ("  ");
		    else         printf ("%d ", (i+1)/3-1);
		putchar ('\n');
	    }
	    printf (" r: ");
	    for (i = n; --i >= 0; )
		printf ("%02x", data[i]);
	    putchar ('\n');

	    /* prompt for and get a new line of input */
	    printf ("%s", prompt);
	    if (fgets (line, sizeof(line), stdin) == NULL)
		exit (0);
	    l = strlen (line)-1;	/* remove trailing '\n' */
	    line[l] = '\0';

	    /* if just type a newline, read again */
	    if (l == 0)
		continue;

	    /* give help if type a lone ? or a line that is too long */
	    if (line[0] == '?' || l > 2*n) {
		printf ("Type a new value of %d hex chars; use blanks to reuse a char.\n", 2*n);
		continue;
	    }

	    /* pad with blanks if line is short */
	    if (l < 2*n)
		sprintf (line+l, "%*s", 2*n-l, "");

	    /* convert to hex in data[] */
	    for (i = n; --i >= 0; ) {
		char c0, c1;
		int d0, d1;

		c1 = line[(n-(i+1))*2];
		if (c1 == ' ')
		    d1 = (((unsigned int)data[i]) >> 4) & 0xf;
		else {
		    d1 = hex (c1);
		    if (d1 < 0) {
			printf ("Bad hex char: %c\n", c1);
			error = 1;
			break;
		    }
		}

		c0 = line[(n-(i+1))*2+1];
		if (c0 == ' ')
		    d0 = data[i] & 0xf;
		else {
		    d0 = hex (c0);
		    if (d0 < 0) {
			printf ("Bad hex char: %c\n", c0);
			error = 1;
			break;
		    }
		}

		data[i] = (d1 << 4) | d0;
	    }

	    /* loop again if error */
	    if (error)
		continue;

	    /* print new data to be written to verify */
	    printf (" w: ");
	    for (i = n; --i >= 0; )
		printf ("%02x", data[i]);
	    putchar ('\n');

	    /* write the data to the device */
	    if (write (fd, data, n) != n) {
		perror ("write");
		exit (1);
	    }
	}
}

usage()
{
	printf ("usage: [/dev/XXX]\n");
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
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: dio.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
