/* code to handle setting the focus motor */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telstatshm.h"
#include "configfile.h"
#include "teled.h"

extern TelStatShm *telstatshmp;

/* home states */
typedef enum {
    HS_IDLE = 0,		/* not active */
    HS_SEEKF,			/* slewing in "forward" direction */
    HS_SEEKB,			/* slewing in "backward" direction */
    HS_HOMEB,			/* going "backward" over home */
    HS_HOMEF,			/* crawling "forward" towards home */
    HS_FILT,			/* waiting to head towards nominal focus */
    HS_DONE,			/* waiting to reach nominal focus */
} HomeState;

static HomeState homestate;	/* current homing state */

/* info from config file */
static int FOCHAVE;		/*  set to 1 if have a focus motor, 0 if not. */
static int FOCMAXVEL;		/*  max motor velocity, usteps/sec */
static int FOCMAXACC;		/*  max motor acceleration, usteps/sec/sec */
static int FOCHOMEDIR;		/*  1 if home is likely positive, 0 if neg */
static double FOCNOMHOME;	/*  microns from home to nominal focus */
static double FOCSCALE;		/*  usteps per micron */
static char FOCAXIS;		/*  focus axis */

static int active;		/* set when we really need to read hardware */

#define FOCHOMEF	(FOCHOMEDIR ? 'm' : 'r')	/* home cmd forwards */
#define FOCHOMEB	(FOCHOMEDIR ? 'r' : 'm')	/* home cmd backwards */

static void focus_cfg(void);

/* return 0 if even have a focus motor at all, else -1 */
int
focus_have()
{
	if (FOCHAVE)
	    return (0);
	    
	telstatshmp->opos = 0.0;
	telstatshmp->orpos = 0;
	telstatshmp->orate = 0.0;
	telstatshmp->opref = 0.0;
	telstatshmp->otarg = 0.0;
	active = 0;
	return (-1);
}

/* reset everything about the focus motor short of finding home.
 * for that, use focus_starthome() and focus_chkhome().
 * return 0 if ok, else -1.
 */
int
focus_reset()
{
	int s = 0;

	/* (re)read the focus config file */
	focus_cfg();

	if (!FOCHAVE)
	    return (0);

	/* limits off, user bit 0 low, stop, init basic params */
	s = pc39_w ("a%c ln bl0 lf st ac%d vl%d", FOCAXIS,FOCMAXACC,FOCMAXVEL);
	if (s < 0)
	    tdlog ("Focus: Error resetting");

	/* init current state */
	active = 1;
	if (focus_readraw() < 0)
	    return (-1);
	telstatshmp->opref = telstatshmp->otarg  = telstatshmp->opos;
	active = 0;

	return (s);
}

/* read and save the current focus position and speed in telstatshmp.
 * return 0 if ok, else -1.
 */
int
focus_readraw()
{
	char back[132];
	int o, n, v;

	if (!FOCHAVE || !active)
	    return (0);

	if (pc39_wr (back, sizeof(back), "a%c rp", FOCAXIS) < 0) {
	    tdlog ("Focus: rp error");
	    return (-1);
	}
	n = atoi(back);
	o = telstatshmp->orpos;

	/* rv just reports abs value */
	if (pc39_wr (back, sizeof(back), "a%c rv", FOCAXIS) < 0) {
	    tdlog ("Focus: rv error");
	    return (-1);
	}
	v = atoi (back);
	if (n < o)
	    v = -v;
	telstatshmp->orate = v/FOCSCALE;
	telstatshmp->orpos = n;
	telstatshmp->opos = n/FOCSCALE;

	return (0);
}

/* no home switch, so just set up some params and say ok.
 * return 0 if ok and underway, else -1.
 * N.B. we thoughtfully return 0 if !FOCHAVE.
 */
int
focus_starthome()
{
	char back[132];

	/* pretend ok if doesn't exist */
	if (!FOCHAVE)
	    return (0);

	/* fake state */
	telstatshmp->otarg  = 0.0;
	telstatshmp->opref  = 0.0;
	homestate = HS_HOMEF;
	active = 1;

	return (0);
}

/* check the focus axis for homing progress.
 * no home switch, so just set state and claim finished.
 * return -1 if error, 0 if making progress, 1 if finished.
 * N.B. we assume focus_starthome() has already been called successfully.
 * N.B. we thoughtfully return 1 if !FOCHAVE.
 */
int
focus_chkhome()
{
	/* pretend to be home if doesn't exist */
	if (!FOCHAVE)
	    return (1);

	telstatshmp->orate = 0.0;
	homestate = HS_DONE;
	active = 0;
	return (1);
}

/* start a relative focus move, in microns.
 * return -1 if error else 0 if ok.
 */
int
focus_set (npstr)
char *npstr;
{
	double dp = atof (npstr);

	if (!FOCHAVE)
	    return (0);

	if (teled_trace)
	    tdlog ("focus_set(%s)", npstr);

	if (pc39_w("a%c vl%d mr%d go",FOCAXIS, FOCMAXVEL, (int)(dp*FOCSCALE))<0)
	    return (-1);

	telstatshmp->opref = telstatshmp->otarg = telstatshmp->opos + dp;

	active = 1;

	return (0);
}

/* return 0 if the focus is currently moving, else -1.
 */
int
focus_check()
{
	return (telstatshmp->orate == 0.0 ? -1 : 0);
}

static void
focus_cfg()
{
	char value[128];

	setConfigFile ("archive/config/focus.cfg");

	readIVal ("FOCHAVE", &FOCHAVE);
	readIVal ("FOCMAXVEL", &FOCMAXVEL);
	readIVal ("FOCMAXACC", &FOCMAXACC);
	readIVal ("FOCHOMEDIR", &FOCHOMEDIR);
	if (FOCHOMEDIR != 0 && FOCHOMEDIR != 1) {
	    fprintf (stderr, "FOCHOMDIR must be 0 or 1: %d\n", FOCHOMEDIR);
	    exit(1);
	}
	readDVal ("FOCNOMHOME", &FOCNOMHOME);
	readDVal ("FOCSCALE", &FOCSCALE);

	if (readCFG ("FOCAXIS", value, sizeof(value)) < 0) {
	    fprintf (stderr, "Can not get Focus axis\n");
	    exit(1);
	}
	FOCAXIS = value[0];
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: focus.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
