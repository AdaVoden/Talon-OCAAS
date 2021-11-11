/* handle the Lights channel */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "strops.h"
#include "telstatshm.h"
#include "running.h"
#include "misc.h"
#include "telenv.h"
#include "cliserv.h"
#include "tts.h"

#include "teled.h"

static void lights_cfg (void);
static void lights_set (int intensity);

static void l_set (int intensity);

static int MAXFLINT;

/* called when we receive a message from the Lights fifo.
 * if msg just ignore.
 */
/* ARGSUSED */
void
lights_msg (msg)
char *msg;
{
	int intensity;

	/* no polling required */
	if (!msg)
	    return;

	/* do reset before checking for `have' to allow for new config file */
	if (strncasecmp (msg, "reset", 5) == 0) {
	    lights_cfg();
	    fifoWrite (Lights_Id, 0, "Reset complete");
	    return;
	}

	/* ignore if none */
	if (telstatshmp->lights < 0)
	    return;

	/* crack command -- including some oft-expected pseudnyms */
	if (sscanf (msg, "%d", &intensity) == 1)
	    lights_set (intensity);
	else if (strncasecmp (msg, "stop", 4) == 0)
	    lights_set (0);
	else if (strncasecmp (msg, "off", 3) == 0)
	    lights_set (0);
	else
	    fifoWrite (Lights_Id, -1, "Unknown command: %.20s", msg);
}

static void
lights_set (int intensity)
{
	if (intensity < 0)
	    fifoWrite (Lights_Id, -2, "Bogus intensity setting: %d", intensity);
	else {
	    if (intensity > MAXFLINT) {
		l_set (MAXFLINT);
		telstatshmp->lights = MAXFLINT;
		fifoWrite (Lights_Id, 0, "Ok, but %d reduced to max of %d",
							intensity, MAXFLINT);
	    } else {
		l_set (intensity);
		telstatshmp->lights = intensity;
		fifoWrite (Lights_Id, 0, "Ok, intensity now %d", intensity);
	    }

	    if (intensity == 0)
		toTTS ("The lights are now off.");
	    else
		toTTS ("The light intensity is now %d.", intensity);
	}
}

static void
lights_cfg()
{
	static char name[] = "MAXFLINT";

	if (read1CfgEntry (1, tdcfn, name, CFG_INT, &MAXFLINT, 0) < 0) {
	    tdlog ("%s: %s not found\n", basenm(tdcfn), name);
	    die();
	}

	if (MAXFLINT <= 0)
	    telstatshmp->lights = -1;	/* none */
	else {
	    telstatshmp->lights = 0;	/* off */
	    l_set (0);
	}
}

/* low level light control */
static void
l_set (int i)
{
#if JSF_HALFM
	switch (i) {
	case 0: pc39_w ("bl6 bl7"); break;
	case 1: pc39_w ("bh6 bl7"); break;
	case 2: pc39_w ("bl6 bh7"); break;
	case 3: pc39_w ("bh6 bh7"); break;
	}
#else
	switch (i) {
	case 0: pc39_w ("bh8 bh9"); break;
	case 1: pc39_w ("bl8 bh9"); break;
	case 2: pc39_w ("bh8 bl9"); break;
	case 3: pc39_w ("bl8 bl9"); break;
	}
#endif
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: lights.c,v $ $Date: 2003/04/15 20:48:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
