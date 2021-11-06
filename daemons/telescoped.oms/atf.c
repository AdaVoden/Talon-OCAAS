/* code to support the ATF, a very special case.
 * only triggered if see "HAVEATF" in telescoped.cfg.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "scan.h"
#include "telstatshm.h"
#include "dio.h"

#include "teled.h"

static int HAVEATF;

/* return 1 if using the ATF, else 0 */
int
atf_have()
{
	return (HAVEATF);
}

/* re-read the config file to see whether we are to use the ATF */
void
atf_cfg()
{
	HAVEATF = 0;
	(void) read1CfgEntry (1, tdcfn, "HAVEATF", CFG_INT, &HAVEATF, 0);
}

/* read the X and Y absolute encoders connected to the DIO 96 */
void
atf_readraw(int *xp, int *yp)
{
	unsigned char bits[12];

	/* set up 8255s as input for ha/dec encoders, output for TG bit */
	dio96_ctrl (0, 0x9b);    /* all lines input on 1st chip */
	dio96_ctrl (1, 0x93);    /* all lines input on 2nd chip but C-hi */

	/* strobe the TG encoders, delay, read */
	dio96_clrbit (44);  /* clear the TG bit */
	dio96_setbit (44);  /* set the TG bit */
	usleep (10000);     /* delay, in us */
	dio96_getallbits (bits);/* read all encoders */
	*xp = ((unsigned)bits[1]<<8) | (unsigned)bits[0];
	*yp = ((unsigned)bits[4]<<8) | (unsigned)bits[3];

	/* we want signed dec values */
	if (*yp > 32768)
	    *yp -= 65536;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: atf.c,v $ $Date: 2003/04/15 20:47:59 $ $Revision: 1.1.1.1 $ $Name:  $"};
