/* code to handle the dome shutter */

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telstatshm.h"
#include "configfile.h"
#include "teled.h"

extern teled_trace;
extern TelStatShm *telstatshmp;

void stopShutter(void);

static int MAXSHUTTERDELAY;	/* max secs to wait for shutter */
static int SHUTTERHAVE;         /* 1 if we have a shutter, 0 if not */

static void readCfg(void);
static void resetShutter(void);

/* return 0 if even have a shutter at all, else -1 */
int
haveShutter()
{
	readCfg();

	if (SHUTTERHAVE)
	    return (0);

	telstatshmp->shutterstatus = SHTR_ABSENT;
	return (-1);
}

/* call this once to init the shutter.
 */
void
initShutter()
{
	resetShutter();
}

/* call whenever to get the dome hardware back to a known, inited state.
 */
static void
resetShutter()
{
	/* read the shutter config file */
	readCfg();

	/* we need output mode 0 for both bytes of port C.
	 * set the remaining ports to input for now.
	 */
	dio24_ctrl (0, 0x92);
	stopShutter();
}

/* start opening the shutter.
 * return an estimate of time to completion, in seconds.
 */
int
openShutter ()
{
	if (telstatshmp->shutterstatus == SHTR_OPEN)
	    return (0);

	dio24_clrbit (20);	/* bit 4 port C */
	dio24_setbit (19);	/* bit 3 port C */

	telstatshmp->shutterstatus = SHTR_OPENING;

	return (MAXSHUTTERDELAY);
}

/* start closing the shutter.
 * return an estimate of time to completion, in seconds.
 */
int
closeShutter ()
{
	if (telstatshmp->shutterstatus == SHTR_CLOSED)
	    return (0);

	dio24_setbit (19);	/* bit 3 port C */
	dio24_setbit (20);	/* bit 4 port C */

	telstatshmp->shutterstatus = SHTR_CLOSING;

	return (MAXSHUTTERDELAY);
}

/* stop moving the shutter.
 * we assume we get this after the prescribed delay, ie, the desired motion
 *   is complete.
 */
void
stopShutter ()
{
	dio24_clrbit (19);	/* bit 3 port C */
	dio24_clrbit (20);	/* bit 4 port C */

	switch (telstatshmp->shutterstatus) {
	case SHTR_OPENING: 	/* FALLTHRU */
	case SHTR_OPEN:
	    telstatshmp->shutterstatus = SHTR_OPEN;
	    break;
	case SHTR_CLOSING:	/* FALLTHRU */
	case SHTR_CLOSED:
	    telstatshmp->shutterstatus = SHTR_CLOSED;
	    break;
	default:
	    telstatshmp->shutterstatus = SHTR_IDLE;
	    break;
	}
}

static void
readCfg()
{
	double d;

	setConfigFile ("archive/config/domed.cfg");

	readDVal ("MAXSHUTTERDELAY", &d);
	MAXSHUTTERDELAY= (int)(d + 0.5);
	readIVal ("SHUTTERHAVE", &SHUTTERHAVE);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: shutter.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
