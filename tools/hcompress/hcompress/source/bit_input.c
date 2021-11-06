/* BIT INPUT ROUTINES */

#include <stdio.h>
#include <stdlib.h>


/* THE BIT BUFFER */

static int buffer;					/* Bits waiting to be input				*/
static int bits_to_go;				/* Number of bits still in buffer		*/


/* INITIALIZE BIT INPUT */

extern void
start_inputing_bits()
{
	/*
	 * Buffer starts out with no bits in it
	 */
	bits_to_go = 0;
}


/* INPUT A BIT */

extern int
input_bit(infile)
FILE *infile;
{
	if (bits_to_go == 0) {						/* Read the next byte if no	*/
		buffer = getc(infile);					/* bits are left in buffer	*/
		if (buffer == EOF) {
			/*
			 * end of file is an error for this application
			 */
			fprintf(stderr, "input_bit: unexpected end-of-file\n");
			exit(-1);
		}
		bits_to_go = 8;
	}
	/*
	 * Return the next bit
	 */
	bits_to_go -= 1;
	return((buffer>>bits_to_go) & 1);
}


/* INPUT N BITS (N must be <= 8) */

extern int
input_nbits(infile,n)
FILE *infile;
int n;
{
int c;

	if (bits_to_go < n) {
		/*
		 * need another byte's worth of bits
		 */
		buffer <<= 8;
		c = getc(infile);
		if (c == EOF) {
			/*
			 * end of file is an error for this application
			 */
			fprintf(stderr, "input_nbits: unexpected end-of-file\n");
			exit(-1);
		}
		buffer |= c;
		bits_to_go += 8;
	}
	/*
	 * now pick off the first n bits
	 */
	bits_to_go -= n;
	return( (buffer>>bits_to_go) & ((1<<n)-1) );
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: bit_input.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
