/* dome control support functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telstatshm.h"
#include "configfile.h"
#include "teled.h"

extern void gray2bin (unsigned gray, int length, unsigned *binp);
extern void tdlog (char *fmt, ...);

extern TelStatShm *telstatshmp;
extern void resetDome(void);

static void readRawDomeEnc (unsigned *bp, unsigned *gp);
static void read_cfg(void);

static double DOMEZERO;		/* az when encoder is 0, rads +E of N */
static double DOMESCALE;	/* rads per encoder change */
static int DOMEHAVE;            /* 1 if we have a dome to control, else 0 */

/* return 0 if even have a dome at all, else -1 */
int
haveDome()
{
	return (DOMEHAVE ? 0 : -1);
}

/* do whatever it takes to be in a position to control the dome hardware */
void
initDome()
{
	if (teled_trace)
	    tdlog ("initDome()\n");

	resetDome();
}

/* reset the dome hardware to a known, initialized state.
 * also reread the config file.
 */
void
resetDome()
{
	if (teled_trace)
	    tdlog ("resetDome()\n");

	read_cfg();

	/* we need output mode 0 for both bytes of port C.
	 * set the remaining ports to input for now.
	 * N.B. HPC camera temperature read from Port B bit 4.
	 */
	dio24_ctrl (0, 0x92);
}

/* stop the dome from rotating */
void
stopDome()
{
	dio24_clrbit (16);  /* LSB of port C */
	dio24_clrbit (17);  /* LSB+1 of port C */

	if (teled_trace > 1)
	    tdlog ("stopDome()\n");
}

/* set state of autodome opertion.
 */
void
autoDome(int whether)
{
	telstatshmp->autodome = whether;
}

/* start the dome rotating from N to E */
void
rotateDomeNE()
{
	/* reset both bits then set desired direction */
	dio24_clrbit (16);  /* LSB of port C */
	dio24_clrbit (17);  /* LSB+1 of port C */
	dio24_setbit (16);  /* LSB of port C */
}

/* start the dome rotating from N to W */
void
rotateDomeNW()
{
	/* reset both bits then set desired direction */
	dio24_clrbit (16);  /* LSB of port C */
	dio24_clrbit (17);  /* LSB+1 of port C */
	dio24_setbit (17);  /* LSB+1 of port C */
}

/* read the encoder and return the current dome position, in rads E of N.
 */
void
readDomeAz (azp)
double *azp;
{
	unsigned int binaz, grayaz;
	double az;

	if (!DOMEHAVE)
	    return;

	readRawDomeEnc (&binaz, &grayaz);
	az = binaz;	/* do math with signed double */
	*azp = DOMEZERO + DOMESCALE*az;

	range (azp, 2*PI);

	if (teled_trace > 2)
	    tdlog ("Dome at %g\n", *azp);

	telstatshmp->domeaz = *azp;
}

/* read the current settings of the dome encoder.
 * return it as a binary value and, just for grins, the original 10-bit gray
 * code value too.
 */
static void
readRawDomeEnc (bp, gp)
unsigned *bp, *gp;
{
	unsigned char bits[12];	/* 4 chips, 3 bytes per chip */

	dio96_ctrl (0, 0x9b);    /* all lines input on 1st chip */
	dio96_ctrl (1, 0x93);    /* all lines input on 2nd chip but C-hi */

	dio96_getallbits (bits);

	/* port C of 1st 8255 is low byte of gray code.
	 * lower two bits of port C of 2nd 8255 are the upper 2 bits.
	 */
	*gp = (((unsigned)bits[5] & 0x3) << 8) | (unsigned)bits[2];

	gray2bin (*gp, 10, bp);
}

/* given an altitude and azimuth, return a "corrected" azimuth to make the
 * given sky position be centered in the shutter due to mount parallax.
 */
void
domeParallax (alt, az, azcor)
double alt, az;
double *azcor;
{
	/* from rct/dome.c
	*azcor = degrad(-28.2 + 42.74*cos(alt) + 0.86*raddeg(az));
	*/

	/* from rlm 2/25/94
	Azdome = Azim + 0.15*cos(Azim+.04)*tan(Elev) - 0.28  radians
	*/
	*azcor = az + 0.15*cos(az+.04)*tan(alt) - 0.28;
	range (azcor, 2*PI);
}

static void
read_cfg()
{
	setConfigFile ("archive/config/domed.cfg");

	readDVal ("DOMEZERO", &DOMEZERO);
	readDVal ("DOMESCALE", &DOMESCALE);
	readIVal ("DOMEHAVE", &DOMEHAVE);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: domectrl.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
