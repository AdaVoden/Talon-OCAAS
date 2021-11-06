/* convert gray code to binary */

/* handy macros to extract and set bits in an unsigned int.
 * in all cases, bits are assumed to be and returned as right-justified.
 *   a is the unsigned int;
 *   n is the bit to twiddle, 0 is the LSB;
 *   v is the value to set it to.
 */
#define	GETBIT(a,n)	(((a) & (1<<(n))) >> (n))
#define	SETBIT(a,n,v)	((a) = ((a) & ~(1<<(n))) | ((v)<<(n)))

/* convert the gray code, g, of l bits to a binary code at *bp.
 * (from Trueblood1985, pg 65)
 */
void
gray2bin (gray, l, binp)
unsigned int gray;	/* gray code */
int l;			/* number of bits in gray code */
unsigned int *binp;	/* place to return corresponding binary code */
{
	unsigned int bin;
	int n;

	/* convery count to bit shift */
	l--;

	bin = 0;
	SETBIT(bin, l, GETBIT(gray,l));
	for (n = l; n > 0; n--) {
	    unsigned int g = GETBIT (gray, n-1);
	    unsigned int b = GETBIT (bin,n);
	    SETBIT (bin, n-1, b ? !g : g);
	}

	*binp = bin;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: gray2bin.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
