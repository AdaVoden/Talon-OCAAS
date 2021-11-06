/* code to handle setting a certain filter.
 * the home switch uses a light we turn on via dio24 bit 24 (set is off).
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telstatshm.h"
#include "configfile.h"
#include "teled.h"

extern TelStatShm *telstatshmp;

/* read filter config file and build a list of filter names at each position */
#define	MAXF	16	/* max filters we can manage */
static char fmap[MAXF];	/* filter code (first character) */
static int nfmap;	/* actual number of filters in use */

static int FHAVE;	/* 1 for filter wheel control, 0 to disable. */
static int FSTEP;	/* usteps per full revolution (or tray travel) */
static int F1STEP;	/* usteps between each filter position */
static int FOFFSET;	/* usteps from home to first filter center */
static int FMAXVEL;	/* max velocity, usteps/sec  */
static int FMAXACC;	/* max acceleration, usteps/sec/sec */
static int FHOMEDIR;	/* 1 if home direction is positive, 0 if neg */
static char FAXIS;	/* motor axis */
static char FDEFLT[20];	/* default position after homing */

static int active;	/* 1 if really have to read the hardware */

static void filt_readcfg(void);
static void filt_name(void);

/* return 0 if even have a filter wheel at all, else -1 */
int
filt_have()
{
	if (FHAVE)
	    return(0);
	
	telstatshmp->filter = ' ';
	telstatshmp->ipos = 0.0;
	telstatshmp->irpos = 0;
	telstatshmp->irate = 0.0;
	telstatshmp->itarg = 0.0;
	telstatshmp->ipref = 0.0;
	active =  0;
	return (-1);
}

/* reset everything about the filter motor short of finding home.
 * for that, use filt_starthome() and filt_chkhome().
 * return 0 if ok, else -1.
 */
int
filt_reset()
{
	int s;

	/* (re)read config file */
	filt_readcfg();

	if (!FHAVE)
	    return(0);

	/* turn off optical switch light source for the home switch */
	dio24_setbit (21);

	/* limits off, bit2 low, init basic params */
	s = pc39_w ("a%c ln bl2 lf st ac%d vl%d", FAXIS, FMAXACC, FMAXVEL);
	if (s < 0)
	    tdlog ("Filter: Error resetting");

	/* init current state */
	active = 1;
	if (filt_readraw() < 0)
	    return (-1);
	telstatshmp->ipref = telstatshmp->itarg = telstatshmp->ipos;
	active = 0;

	return (s);
}

/* read and save the current filter position, speed and name in telstatshmp.
 * return 0 if ok, else -1.
 */
int
filt_readraw()
{
	char back[132];
	int o, n, v;

	if (!FHAVE || !active)
	    return(0);

	if (nohw) {
	    if (telstatshmp->irate != 0) {
		filt_name();
		sleep (2);
		telstatshmp->irate = 0;
		telstatshmp->irpos=(int)floor(telstatshmp->itarg/2/PI*FSTEP+.5);
		telstatshmp->ipos = telstatshmp->itarg;
		filt_name();
	    }
	    return (0);
	}

	if (pc39_wr (back, sizeof(back), "a%c rp", FAXIS) < 0) {
	    tdlog ("Focus: rp error");
	    return (-1);
	}
	n = atoi(back);
	o = telstatshmp->irpos;

	/* rv just reports abs value */
	if (pc39_wr (back, sizeof(back), "a%c rv", FAXIS) < 0) {
	    tdlog ("Focus: rv error");
	    return (-1);
	}
	v = atoi (back);

	/* right after setting a move, speed can be finite before any
	 * change in position. how to tell sign of speed then?? 
	 */

	if (n < o)
	    v = -v;
	telstatshmp->irate = (2*PI)*v/FSTEP;
	telstatshmp->irpos = n;
	telstatshmp->ipos = (2*PI)*n/FSTEP;

	/* set name */
	filt_name();

	return (0);
}

/* start the filter motor moving to find its home.
 * return 0 if ok and underway, else -1.
 */
int
filt_starthome()
{
	int homeback = FHOMEDIR ? -FSTEP/72 : FSTEP/72;	/* 5 degrees */
	char homedir = FHOMEDIR ? 'm' : 'r';
	int seekv;
	int s;

	if (!FHAVE)
	    return(0);

	/* turn on optical switch light source for home switch */
	dio24_clrbit (21);

	/* init, go home fast, then go back and approach more slowly */
	seekv = FMAXVEL > 1000 ? 1000 : FMAXVEL;
	s = pc39_w ("a%c ac%d vl%d h%c0  vl%d mr%d go  vl%d h%c0",
					    FAXIS, FMAXACC, FMAXVEL, homedir,
					    FMAXVEL/4, homeback,
					    seekv, homedir);

	/* append command to go to default position */
	if (s == 0)
	    s = filt_set (FDEFLT);
	if (s < 0)
	    tdlog ("Filter: error starting home");

	return (s);
}

/* check the filter motor for progress going home.
 * return -1 if trouble, 0 if making progress, 1 if home.
 * N.B. we assume filt_starthome() has already been called successfully.
 */
int
filt_chkhome()
{
	if (telstatshmp->irate != 0.0)
	    return (0);

	/* turn off optical switch light source for the home switch */
	dio24_setbit (21);

	if (fabs(telstatshmp->ipos - telstatshmp->itarg)/(2*PI)*FSTEP < 1) {
	    active = 0;
	    return (1);
	}
	return (-1);
}

/* start a filter to be positioned, by name,
 * just look for first char to match, and that without regard to case.
 * return 0 of ok, else fill name[] with reason why and return -1.
 */
int
filt_set (name)
char *name;
{
	int newpos;
	char f;

	if (!FHAVE)
	    return(0);

	/* search for index with given name, ignoring case */
	f = *name;
	if (islower(f))
	    f = toupper(f);
	for (newpos = 0; newpos < nfmap; newpos++)
	    if (fmap[newpos] == f)
		break;
	if (newpos == nfmap) {
	    if (teled_trace)
		tdlog ("filt_set(): no filter named %s", name);
	    (void) sprintf (name, "No filtter named %s", name);
	    return (-1);
	}

	/* convert to steps */
	newpos = FOFFSET + newpos*F1STEP;

	/* go straight to desired filter offset */
	telstatshmp->ipref = telstatshmp->itarg = (2*PI)*newpos/FSTEP;
	(void) pc39_w ("a%c vl%d ma%d go", FAXIS, FMAXVEL, newpos);

	if (teled_trace)
	    tdlog ("filt_set(): name=`%c' newpos=%d", name[0], newpos);

	/* just to indicate effort has begun */
	telstatshmp->irate = (2*PI)*FMAXVEL/FSTEP;

	active = 1;
	return (0);
}

/* set telstatshmp->filter based on current position.
 */
static void
filt_name()
{
	if (telstatshmp->irate > 0.0)
	    telstatshmp->filter = '>';
	else if (telstatshmp->irate < 0.0)
	    telstatshmp->filter = '<';
	else {
	    int fs = (telstatshmp->irpos - FOFFSET)/F1STEP;
	    int fm = (telstatshmp->irpos - FOFFSET)%F1STEP;
	    if (fs < 0 || fs >= nfmap || fm != 0)
		telstatshmp->filter = '?';
	    else
		telstatshmp->filter = fmap[fs];
	}
}

static void
filt_readcfg()
{
	char name[128], value[128];
	char f;
	int i;

	/* prepare to access the filter config file */
        setConfigFile ("archive/config/filter.cfg");

	/* read desired default */
	if (readCFG ("FDEFLT", FDEFLT, sizeof(FDEFLT)) < 0) {
	    fprintf (stderr, "%s missing\n", "FDEFLT");
	    exit(1);
	}

	/* fetch the number of filters */
	readIVal ("NFILT", &nfmap);
	if (nfmap < 1 || nfmap > MAXF) {
	    fprintf (stderr, "NFILT must be in range 1..%d: %d\n", MAXF, nfmap);
	    exit(1);
	}

	/* read in the filter names in order of position */
	for (i = 0; i < nfmap; i++) {
	    sprintf (name, "F%dNAME", i);
	    if (readCFG (name, value, sizeof(value)) < 0) {
		fprintf (stderr, "%s missing\n", name);
		exit(1);
	    }
	    f = value[0];
	    if (islower(f))
		f = toupper(f);
	    fmap[i] = f;
	}

	/* read the other params */
	readIVal ("FHAVE", &FHAVE);
	readIVal ("FSTEP", &FSTEP);
	readIVal ("F1STEP", &F1STEP);
	readIVal ("FOFFSET", &FOFFSET);
	readIVal ("FMAXACC", &FMAXACC);
	readIVal ("FMAXVEL", &FMAXVEL);
	readIVal ("FHOMEDIR", &FHOMEDIR);

	if (readCFG ("FAXIS", value, sizeof(value)) < 0) {
	    fprintf (stderr, "Can not get Filter axis\n");
	    exit(1);
	}
	FAXIS = value[0];
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: filter.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
