#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* usage:
 * to encode: csi 1 <key> < plain > crypt
 * to decode: csi 0 <key> < crypt > plain
 */

static unsigned mkhash (char *s);
static unsigned ror4 (unsigned x);

#define	RMASK	0x29	/* emit random if any of these bits are on */

int
main (int ac, char *av[]) 
{
	int enc = atoi(av[1]);
	char *key = av[2];
	int kl = strlen(key);
	unsigned hash = mkhash(key);
	unsigned char c0 = 3;
	int c;
	int i;

	srandom (time(NULL));

	for (i = 0; (c = getchar()) != EOF; i++) {
	    int rot = (((int)c0 + (int)key[i%kl] + hash)>>3) & 0x7;

	    c0 = c;
	    if (rot == 0)
		rot = 5;
	    if (enc) {
		c = (c0 << rot) | (c0 >> (8-rot));
		putchar (c & 0xff);
		if (c & RMASK)
		    putchar (random());
	    } else {
		if (c & RMASK)
		    (void) getchar();
		c = (c0 >> rot) | (c0 << (8-rot));
		putchar (c & 0xff);
		c0 = c;
	    }

	    hash = ror4(hash);
	}

	return (0);
}

/* K&R 2nd Ed pg 144 */
static unsigned
mkhash (char *s)
{
	unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
	    hashval = *s + 31 * hashval;

	return (hashval);
}

/* rotate right 4 */
static unsigned
ror4 (unsigned x)
{
	int bits = sizeof(unsigned)*8;
	return ((x>>4) | ((x&0xf)<<(bits-4)));
}
