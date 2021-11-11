/* swap_bytes.c	Swap bytes in I*2 data
 *
 * Programmer: R. White		Date: 17 April 1992
 */

/*
 * test to see if byte swapping is necessary on this machine
 */
extern int
test_swap()
{
union {
	short s;
	char c[2];
	} u;

	u.c[0] = 0;
	u.c[1] = 1;
	return (u.s != 1);
}

/*
 * swap bytes in pairs in A
 */
extern void
swap_bytes(a,n)
unsigned char a[];
int n;
{
int i;
unsigned char tmp;

	for (i=0; i < n-1; i += 2) {
		tmp = a[i];
		a[i] = a[i+1];
		a[i+1] = tmp;
	}
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: swap_bytes.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
