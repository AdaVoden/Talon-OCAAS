/* handle the dome and shutter.
 *
 * functions that respond directly to fifos begin with dome_.
 * middle-layer support functions begin with d_ or s_.
 * low-level implementation functions begin with draw_ or sraw_
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "strops.h"
#include "telstatshm.h"
#include "running.h"
#include "rot.h"
#include "misc.h"
#include "telenv.h"
#include "cliserv.h"
#include "tts.h"

#include "teled.h"

#if JSF_HALFM
static char rttyname[] = "/dev/ttyS2";	/* name of tty connection */

typedef unsigned char Byte;

static Byte csMake (Byte msg[], int n);
static void writeCmd (char cmd[], int n);
static int readBack (char buf[]);
static int openTTY (void);

static int rttyfd;		/* roof tty */

/* basic command sequences */
static char emg_cmd[] = "@00WD001000010000";
static char open_cmd[] = "@00WD001002000000";
static char close_cmd[] = "@00WD001001000000";
static char read_cmd[] = "@00RD00000002";
#endif

/* config entries */
static char	DOMEAXIS;
static double	DOMETOL;
static double	DOMETO;
static double	DOMEZERO;
static double	DOMESTEP;
static int	DOMESIGN;
static int	DOMEHOMELOW;
static int	DOMEPOSSIDE;
static double	DOMEMOFFSET;
static double	SHUTTERTO;
static int	SHUTTERFB;
static double	SHUTTERAZ;
static double	SHUTTERAZTOL;

static double	shutter_to;	/* mjd when shutter operation will timeout */
static double	dome_to;	/* mjd when dome operation will timeout */

/* the current activity, if any */
static void (*active_func) (int first, ...);

/* one of these... */
static void dome_poll(void);
static void dome_reset(int first, ...);
static void dome_open(int first, ...);
static void dome_close(int first, ...);
static void dome_autoOn(int first, ...);
static void dome_autoOff(int first, ...);
static void dome_home(int first, ...);
static void dome_setaz(int first, ...);
static void dome_stop(int first, ...);
static void dome_jog (int first, ...);

/* helped along by these... */
static int d_emgstop(char *msg);
static int d_chkWx(char *msg);
static void d_readpos (void);
static void d_auto (void);
static double d_telaz(void);
static int d_athome(void);
static void d_cw(void);
static void d_ccw(void);
static void d_move(void);
static void d_stop(void);
static int d_dir (void);
static int d_onTarget(double tol);
static int d_toTAZ (double tol);
static void initCfg(void);

/* and in turn these ... */
static int sdraw_emgstop(void);
static int sraw_active(void);
static void sraw_stop(void);
static int sraw_open(void);
static int sraw_close(void);
static void draw_cw(void);
static void draw_ccw(void);
static void draw_stop(void);
static int draw_readpos (void);
static int draw_readhome (void);
static void draw_markhome(void);

/* handy shortcuts */
#define	DS		(telstatshmp->domestate)
#define	SS		(telstatshmp->shutterstate)
#define	AD		(telstatshmp->autodome)
#define	AZ		(telstatshmp->domeaz)
#define	TAZ		(telstatshmp->dometaz)
#define	SMOVING		(SS == SH_OPENING || SS == SH_CLOSING)
#define	DMOVING		(DS == DS_ROTATING || DS == DS_HOMING)
#define	DHAVE		(DS != DS_ABSENT)
#define	SHAVE		(SS != SH_ABSENT)


/* called when we receive a message from the Dome fifo plus periodically with
 *   !msg to just update things.
 */
/* ARGSUSED */
void
dome_msg (msg)
char *msg;
{
	char jog_dir[2];
	double az;

	/* do reset before checking for `have' to allow for new config file */
	if (msg && strncasecmp (msg, "reset", 5) == 0) {
	    dome_reset(1);
	    return;
	}

	/* worth it? */
	if (!DHAVE && !SHAVE) {
	    if (msg)
		fifoWrite (Dome_Id, 0, "Ok, but dome really not installed");
	    return;
	}

	/* top priority are emergency stop and weather alerts */
	if (d_emgstop(msg) || d_chkWx(msg))
	    return;

	/* handle normal messages and polling */
	if (!msg)
	    dome_poll();
	else if (strncasecmp (msg, "stop", 4) == 0)
	    dome_stop(1);
	else if (strncasecmp (msg, "open", 4) == 0)
	    dome_open(1);
	else if (strncasecmp (msg, "close", 5) == 0)
	    dome_close(1);
	else if (strncasecmp (msg, "auto", 4) == 0)
	    dome_autoOn(1);
	else if (strncasecmp (msg, "off", 3) == 0)
	    dome_autoOff(1);
	else if (strncasecmp (msg, "home", 4) == 0)
	    dome_home(1);
	else if (sscanf (msg, "Az:%lf", &az) == 1)
	    dome_setaz (1, az);
	else if (sscanf (msg, "j%1[0+-]", jog_dir) == 1)
	    dome_jog (1, jog_dir[0]);
	else {
	    fifoWrite (Dome_Id, -1, "Unknown command: %.20s", msg);
	    dome_stop (1);	/* default for any unrecognized message */
	}
}

/* maintain current action */
static void
dome_poll ()
{
	if (active_func)
	    (*active_func)(0);
	else if (DHAVE) {
	    if (AD)
		d_auto();
	    else
		d_readpos();
	}
}

/* read config files, stop dome; don't mess much with shutter state */
static void
dome_reset (int first, ...)
{
	Now *np = &telstatshmp->now;

	if (first) {
	    initCfg();
	    if (DHAVE) {
		d_stop();
		dome_to = mjd + DOMETO;
	    }
	    if (SHAVE) {
		sraw_stop();
		if (SMOVING)
		    SS = SH_IDLE;
	    }
	    active_func = dome_reset;
	}

	if (DHAVE) {
	    d_readpos();
	    if (DS != DS_STOPPED) {
		if (mjd > dome_to) {
		    fifoWrite (Dome_Id, -2, "Reset timed out");
		    d_stop();	/* try again? */
		    active_func = NULL;
		}
		return;
	    }
	}

	active_func = NULL;
	fifoWrite (Dome_Id, 0, "Reset complete");
}

/* open shutter or roof */
static void
dome_open (int first, ...)
{
	Now *np = &telstatshmp->now;
	int to;

	/* nothing to do if no shutter */
	if (!SHAVE) {
	    fifoWrite (Dome_Id, -3, "No shutter to open");
	    return;
	}

	/* get dome in place first, if have one */
	if (DHAVE) {
	    if (first) {
		/* target is shutter power az */
		TAZ = SHUTTERAZ;
		fifoWrite (Dome_Id, 1, "Aligning Dome for shutter power");
		toTTS ("The dome is rotating to the shutter position.");

		/* set new state */
		dome_to = mjd + DOMETO;
		active_func = dome_open;
		AD = 0;
	    }

	    /* fall through only if ready for shutter power */
	    if (!d_toTAZ(SHUTTERAZTOL)) {
		if (mjd > dome_to) {
		    fifoWrite (Dome_Id, -4, "Open timed out moving dome");
		    d_stop();
		    active_func = NULL;
		}
		return;
	    }
	}

	/* get here only when know dome is absent or stopped and in place */

	/* initiate open if not under way */
	if (SS != SH_OPENING) {
	    fifoWrite (Dome_Id, 2, "Starting roof opening");
	    toTTS ("The roof is now opening.");
	    if (sraw_open() < 0)
		return;
	    SS = SH_OPENING;
	    shutter_to = mjd + SHUTTERTO;
	    active_func = dome_open;
	    return;
	}

	/* check progress */
	to = mjd >= shutter_to;
	if (nohw || (SHUTTERFB && !sraw_active()) || (!SHUTTERFB && to)) {
	    fifoWrite (Dome_Id, 0, "Roof open complete");
	    toTTS ("The roof is now open.");
	    sraw_stop();
	    SS = SH_OPEN;
	    active_func = NULL;
	} else if (SHUTTERFB && to) {
	    fifoWrite (Dome_Id, -5, "Roof open timed out");
	    sraw_stop();
	    SS = SH_IDLE;
	    active_func = NULL;
	}
}

/* close shutter or roof */
static void
dome_close (int first, ...)
{
	Now *np = &telstatshmp->now;
	int to;

	/* nothing to do if no shutter but don't panic */
	if (!SHAVE) {
	    fifoWrite (Dome_Id, 0, "Ok, but really no shutter to close");
	    return;
	}

	/* get dome in place first, if have one */
	if (DHAVE) {
	    if (first) {
		/* target is shutter power az */
		TAZ = SHUTTERAZ;
		fifoWrite (Dome_Id, 3, "Aligning Dome for shutter power");
		toTTS ("The dome is rotating to the shutter position.");

		/* set new state */
		dome_to = mjd + DOMETO;
		active_func = dome_close;
		AD = 0;
	    }

	    /* fall through only if ready for shutter power */
	    if (!d_toTAZ(SHUTTERAZTOL)) {
		if (mjd > dome_to) {
		    fifoWrite (Dome_Id, -7, "Close timed out moving dome");
		    d_stop();
		    active_func = NULL;
		}
		return;
	    }
	}

	/* get here only when know dome is absent or stopped and in place */

	/* initiate close if not under way */
	if (SS != SH_CLOSING) {
	    fifoWrite (Dome_Id, 4, "Starting roof closing");
	    toTTS ("The roof is now closing.");
	    if (sraw_close() < 0)
		return;
	    SS = SH_CLOSING;
	    shutter_to = mjd + SHUTTERTO;
	    active_func = dome_close;
	    return;
	}

	/* check progress */
	to = mjd >= shutter_to;
	if (nohw || (SHUTTERFB && !sraw_active()) || (!SHUTTERFB && to)) {
	    fifoWrite (Dome_Id, 0, "Roof close complete");
	    toTTS ("The roof is now closed.");
	    sraw_stop();
	    SS = SH_CLOSED;
	    active_func = NULL;
	} else if (SHUTTERFB && to) {
	    fifoWrite (Dome_Id, -8, "Roof close timed out");
	    sraw_stop();
	    SS = SH_IDLE;
	    active_func = NULL;
	}
}

/* activate autodome lock */
/* ARGSUSED */
static void
dome_autoOn (int first, ...)
{
	if (!DHAVE) {
	    fifoWrite (Dome_Id, 0, "Ok, but no dome really");
	} else {
	    /* just set the flag, let poll do the work */
	    AD = 1;
	    fifoWrite (Dome_Id, 0, "Auto dome on");
	}
}

/* deactivate autodome lock */
/* ARGSUSED */
static void
dome_autoOff (int first, ...)
{
	if (!DHAVE) {
	    fifoWrite (Dome_Id, 0, "Ok, but no dome really");
	} else {
	    /* just stop and reset the flag */
	    AD = 0;
	    d_stop();
	    fifoWrite (Dome_Id, 0, "Auto dome off");
	}
}

/* find dome home */
static void
dome_home (int first, ...)
{
	Now *np = &telstatshmp->now;

	if (!DHAVE) {
	    fifoWrite (Dome_Id, 0, "Ok, but really no dome to home");
	    return;
	}

	if (first) {
	    /* start moving towards desired side of home */
	    if (DOMEPOSSIDE)
		d_cw();
	    else
		d_ccw();

	    /* set timeout and new state */
	    dome_to = mjd + DOMETO;
	    active_func = dome_home;
	    DS = DS_HOMING;
	    TAZ = DOMEZERO;
	    AD = 0;
	    toTTS ("The dome is seeking the home position.");
	}

	/* poll for home */
	if (d_athome()) {
	    /* found it -- reset encoder now and stop */
	    draw_markhome();
	    d_stop();
	    fifoWrite (Dome_Id, 0, "Found home");
	    toTTS ("The dome has found the home position.");
	    active_func = NULL;
	} else if (mjd > dome_to) {
	    d_stop();
	    fifoWrite (Dome_Id, -9, "Failed to find home");
	    active_func = NULL;
	}
}

/* move to the given azimuth. also turns off Auto mode. */
static void
dome_setaz (int first, ...)
{
	Now *np = &telstatshmp->now;

	/* nothing to do if no dome */
	if (!DHAVE) {
	    fifoWrite (Dome_Id, -10, "No dome to turn");
	    return;
	}

	if (first) {
	    va_list ap;
	    double taz;

	    /* fetch new target az */
	    va_start (ap, first);
	    taz = va_arg (ap, double);
	    va_end (ap);

	    /* prep */
	    range (&taz, 2*PI);
	    TAZ = taz;
	    d_stop();
	    AD = 0;
	    dome_to = mjd + DOMETO;
	    active_func = dome_setaz;
	    toTTS ("The dome is rotating towards the %s.", cardDirLName (TAZ));
	}

	/* keep trying until in tolerance and stopped or time out */
	if (d_toTAZ(DOMETOL)) {
	    fifoWrite (Dome_Id, 0, "Azimuth command complete");
	    toTTS ("The dome is now pointing to the %s.", cardDirLName (AZ));
	    active_func = NULL;
	} else if (mjd > dome_to) {
	    d_stop();
	    fifoWrite (Dome_Id, -11, "Azimuth command timed out");
	    active_func = NULL;
	}
}

/* stop everything */
static void
dome_stop (int first, ...)
{
	Now *np = &telstatshmp->now;

	if (first) {
	    if (DHAVE) {
		AD = 0;
		d_stop();
		dome_to = mjd + DOMETO;
	    }
	    if (SHAVE) {
		sraw_stop();
		if (SMOVING)
		    SS = SH_IDLE;
	    }
	    active_func = dome_reset;
	}

	if (DHAVE) {
	    d_readpos();
	    if (DS != DS_STOPPED) {
		if (mjd > dome_to) {
		    fifoWrite (Dome_Id, -12, "Stop timed out");
		    d_stop();	/* try again? */
		    active_func = NULL;
		}
		return;
	    }
	}

	active_func = NULL;
	fifoWrite (Dome_Id, 0, "Stop complete");
	toTTS ("The roof is now stopped.");
}

/* jog: + means CW, - means CCW, 0 means stop */
static void
dome_jog (int first, ...)
{
	static int nohwdir;
	char dircode;
	va_list ap;

	if (!DHAVE) {
	    fifoWrite (Dome_Id, -13, "No Dome to jog");
	    return;
	}

	if (first) {

	    /* fetch direction code */
	    va_start (ap, first);
	    dircode = (char) va_arg (ap, int);
	    va_end (ap);

	    /* no more AD */
	    AD = 0;

	    /* do it */
	    switch (dircode) {
	    case '+':
		fifoWrite (Dome_Id, 5, "Paddle command CW");
		toTTS ("The dome is rotating clockwise.");
		d_cw();
		nohwdir = 1;
		active_func = dome_jog;
		break;
	    case '-':
		fifoWrite (Dome_Id, 6, "Paddle command CCW");
		toTTS ("The dome is rotating counter clockwise.");
		d_ccw();
		nohwdir = -1;
		active_func = dome_jog;
		break;
	    case '0':
		fifoWrite (Dome_Id, 7, "Paddle command stop");
		draw_stop();
		nohwdir = 0;
		active_func = NULL;
		break;
	    default:
		fifoWrite (Dome_Id, -14, "Bogus jog code: %c", dircode);
		nohwdir = 0;
		active_func = NULL;
		break;
	    }

	    return;
	}

	d_readpos();

	if (nohw) {
	    if (nohwdir > 0)
		d_cw();
	    else if (nohwdir < 0)
		d_ccw();
	    else if (nohwdir == 0)
		active_func = NULL;
	}
}

/* middle-layer support functions */

/* check the emergency stop bit.
 * while on, stop everything and return 1, else return 0
 */
static int
d_emgstop(char *msg)
{
	int on = (DMOVING || SMOVING) && sdraw_emgstop();

	if (!on)
	    return (0);

	if (msg || (active_func && active_func != dome_stop))
	    fifoWrite (Dome_Id,
			-15, "Command cancelled.. emergency stop is active");

	if (active_func != dome_stop && DS != DS_STOPPED) {
	    fifoWrite (Dome_Id, 8, "Emergency stop asserted -- stopping dome");
	    AD = 0;
	    dome_stop (1);
	}

	dome_poll();

	return (1);
}

/* if a weather alert is in progress respond and return 1, else return 0 */
static int
d_chkWx(char *msg)
{
	WxStats *wp = &telstatshmp->wxs;
	int wxalert = (time(NULL) - wp->updtime < 30) && wp->alert;

	if (!wxalert || !SHAVE)
	    return(0);

	if (msg || (active_func && active_func != dome_close))
	    fifoWrite (Dome_Id,
			-16, "Command cancelled.. weather alert in progress");

	if (active_func != dome_close && SS != SH_CLOSED) {
	    fifoWrite (Dome_Id, 9, "Weather alert asserted -- closing roof");
	    AD = 0;
	    dome_close (1);
	}

	dome_poll();

	return (1);
}

/* read current position and set AZ and DS */
static void
d_readpos ()
{
	double az;
	int stopped;

	/* read fresh */
	if (nohw) {
	    az = TAZ;
	} else {
	    int pos = draw_readpos()*DOMESIGN;
	    az = (2*PI)*pos/DOMESTEP + DOMEZERO;
	}
	range (&az, 2*PI);
	stopped = delra(az-AZ) <= (1.5*2*PI)/DOMESTEP;

	/* update */
	DS = stopped ? DS_STOPPED : DS_ROTATING;
	AZ = az;
}

/* return the az the dome should be for the desired telescope information */
static double
d_telaz()
{
#if 0 // STO: 2002-09-13 Removed this code to match CSI approach.  This doesn't work correctly; just slave to telescope az

	TelAxes *tap = &telstatshmp->tax;	/* scope orientation */
	double Y;	/* scope's "dec" angle to scope's pole */
	double X;	/* scope's "ha" angle, canonical from south */
	double Z;	/* angle from zenith to scope pole = DT - lat */
	double p[3];	/* point to track */
	double Az;	/* dome az to be found */
	
	/* coord system: +z to zenith, +y south, +x west. */
	Y = PI/2 - (DMOT->dpos + tap->YC);
	X = (HMOT->dpos - tap->XP) + PI;
	Z = tap->DT - telstatshmp->now.n_lat;

	/* init straight up, along z */
	p[0] = p[1] = 0.0; p[2] = 1.0;

	/* translate along x by mount offset */
	p[0] += (tap->GERMEQ && tap->GERMEQ_FLIP) ? -DOMEMOFFSET : DOMEMOFFSET;

	/* rotate about x by -Y to affect scope's DMOT position */
	rotx (p, -Y);

	/* rotate about z by X to affect scope's HMOT position */
	rotz (p, X);

	/* rotate about x by Z to affect scope's tilt from zenith */
	rotx (p, Z);

	/* dome az is now projection onto xy plane, angle E of N */
	Az = atan2 (-p[0], -p[1]);
	if (Az < 0)
	    Az += 2*PI;
	
	return (Az);
#endif
	if(telstatshmp->telstate == TS_TRACKING) {
		return(telstatshmp->Caz);
	}
	return(telstatshmp->Daz); // NOTE: Does not account for domeoffset
}

/* read home switch -- return 1 if home else 0 */
static int
d_athome()
{
	return (nohw || draw_readhome() == DOMEHOMELOW);
}

/* initiate a stop */
static void
d_stop(void)
{
	draw_stop();
}

/* start cw */
static void
d_cw()
{
	if (nohw) {
	    TAZ += degrad(10.) * telstatshmp->dt/1000.;
	    range (&TAZ, 2*PI);
	} else
	    draw_cw();
}

/* start ccw */
static void
d_ccw()
{
	if (nohw) {
	    TAZ -= degrad(10.) * telstatshmp->dt/1000.;
	    range (&TAZ, 2*PI);
	} else
	    draw_ccw();
}

/* initiate shortest move from domeaz to dometaz */
static void
d_move(void)
{
	switch (d_dir()) {
	case  1: d_cw();  break;
	case -1: d_ccw(); break;
	case  0: break;
	}
}

/* return 1 if shortest direction from domeaz to dometaz is cw, -1 if ccw,
 * or 0 if equal
 */
static int
d_dir (void)
{
	double az = AZ;
	double taz = TAZ;

	while (taz < az)
	    taz += 2*PI;

	return (taz == az ? 0 : (taz < az + PI ? 1 : -1));
}

/* return 1 if TAZ is within tol of AZ, else 0 */
static int
d_onTarget(double tol)
{
	return (delra(TAZ - AZ) <= tol);
}

/* keep AZ within DOMETOL of telaz() */
static void
d_auto()
{
	TAZ = d_telaz();
	(void) d_toTAZ (DOMETOL);
}

/* keep moving towards TAZ.
 * return 1 when there and stopped, else 0
 */
static int
d_toTAZ (double tol)
{
	d_readpos();
	if (d_onTarget(tol)) {
	    if (DS == DS_STOPPED)
		return (1);
	    else
		draw_stop();
	} else {
	    if (DS == DS_STOPPED)
		d_move();
	}
	return (0);
}

/* (re) read the dome confi file */
static void
initCfg()
{
#define NDCFG   (sizeof(dcfg)/sizeof(dcfg[0]))

	static int DOMEHAVE;
	static int SHUTTERHAVE;

	static CfgEntry dcfg[] = {
	    {"DOMEHAVE",	CFG_INT, &DOMEHAVE},
	    {"DOMEAXIS",	CFG_STR, &DOMEAXIS, 1},
	    {"DOMETOL",		CFG_DBL, &DOMETOL},
	    {"DOMETO",		CFG_DBL, &DOMETO},
	    {"DOMEZERO",	CFG_DBL, &DOMEZERO},
	    {"DOMESTEP",	CFG_DBL, &DOMESTEP},
	    {"DOMESIGN",	CFG_INT, &DOMESIGN},
	    {"DOMEHOMELOW",	CFG_INT, &DOMEHOMELOW},
	    {"DOMEPOSSIDE",	CFG_INT, &DOMEPOSSIDE},
	    {"DOMEMOFFSET",	CFG_DBL, &DOMEMOFFSET},
	    {"SHUTTERHAVE",	CFG_INT, &SHUTTERHAVE},
	    {"SHUTTERTO",	CFG_DBL, &SHUTTERTO},
	    {"SHUTTERFB",	CFG_INT, &SHUTTERFB},
	    {"SHUTTERAZ",	CFG_DBL, &SHUTTERAZ},
	    {"SHUTTERAZTOL",	CFG_DBL, &SHUTTERAZTOL},
	};
	int n;

	/* read the file */
	n = readCfgFile (1, dcfn, dcfg, NDCFG);
	if (n != NDCFG) {
	    cfgFileError (dcfn, n, (CfgPrFp)tdlog, dcfg, NDCFG);
	    die();
	}

	if (abs(DOMESIGN) != 1) {
	    fprintf (stderr, "DOMESIGN must be +-1\n");
	    die();
	}

	/* let user specify neg */
	range (&DOMEZERO, 2*PI);

	/* we want in days */
	DOMETO /= SPD;
	SHUTTERTO /= SPD;

#if JSF_HALFM	
	/* initialize the dome tty, if it doesn't work, don't allow roof control */
    if(SHUTTERHAVE) {
	   rttyfd = openTTY();
       if(rttyfd < 0) {
	    SHUTTERHAVE = 0;
	   }
    }
#endif

	/* some effect shm -- but try not to disrupt already useful info */
	if (!DOMEHAVE) {
	    DS = DS_ABSENT;
	} else if (DS == DS_ABSENT) {
	    d_stop();
	    DS = DS_STOPPED;
	}
	if (!SHUTTERHAVE)
	    SS = SH_ABSENT;
	else if (SS == SH_ABSENT)
	    SS = SH_IDLE;

	/* no auto any more */
	AD = 0;
}


/* the *really* low-level functions */
#if !JSF_HALFM
// NORMAL PC39 version

/* return 1 if the emergency stop line is true, else 0 */
static int
sdraw_emgstop()
{
	char buf[32];
	int hex;

	pc39_wr (buf, sizeof(buf), "bx");
	sscanf (buf, "%x", &hex);
	return (!!(hex & 0x4));
}

/* return 1 if the shutter is trying, else 0. */
static int
sraw_active()
{
	char buf[32];
	int hex;

	pc39_wr (buf, sizeof(buf), "bx");
	sscanf (buf, "%x", &hex);
	return (!!(hex & 0x1));
}

/* issue a low-level shutter stop */
static void
sraw_stop()
{
	pc39_w ("bh12 bh13");
}

/* issue a low-level command to open the shutter */
static int
sraw_open()
{
	pc39_w ("bh13 bl12");
    return 0;
}

/* issue a low-level command to close the shutter */
static int
sraw_close()
{
	pc39_w ("bh12 bl13");
    return 0;
}

/* issue command to rotate cw */
static void
draw_cw()
{
	pc39_w ("bh11 bl10");
}

/* issue command to rotate ccw */
static void
draw_ccw()
{
	pc39_w ("bh10 bl11");
}

/* issue dome stop */
static void
draw_stop()
{
	pc39_w ("bh10 bh11");
}

/* read raw encoder */
static int
draw_readpos()
{
	char buf[32];
	pc39_wr (buf, sizeof(buf), "a%c re", DOMEAXIS);
	return (atoi(buf));
}

/* read raw home bit -- return 1 if low else 0 */
static int
draw_readhome()
{
	char buf[32];
	int hex;

	pc39_wr (buf, sizeof(buf), "bx");
	sscanf (buf, "%x", &hex);
	return (!!(hex & 0x2));
}

/* save the current location as the home location */
static void
draw_markhome()
{
	pc39_w ("a%c lp0", DOMEAXIS);
}
#else
// JSF HALFM TTY ROOF CONTROLLER
/* return 1 if the emergency stop line is true, else 0 */
static int
sdraw_emgstop()
{
	return (0);
}

/* return 1 if the shutter is trying, else 0. */
static int
sraw_active()
{
	char buf[256];
	int l;

	do {
	    writeCmd (read_cmd, sizeof(read_cmd)-1);
	    l = readBack (buf);
	} while (l < 15);
	if (SS == SH_OPENING)
	    return (!((buf[14]-'0') & 0x8));
	if (SS == SH_CLOSING)
	    return (!((buf[13]-'0') & 0x4));
	return (0);
}

/* issue a low-level shutter stop */
static void
sraw_stop()
{
	char buf[256];
	int l;

	do {
	    writeCmd (emg_cmd, sizeof(emg_cmd)-1);
	    l = readBack (buf);
	} while (l < 7 || buf[5] != '0' || buf[6] != '0');
}

/* issue a low-level command to open the shutter.
 * return -1 if trouble else 0
 */
static int
sraw_open()
{
	char buf[256];
	int l;

	/* check for manual mode */
	writeCmd (read_cmd, sizeof(read_cmd)-1);
	l = readBack (buf);
	if (l < 11 || ((buf[10]-'0') & 0x2)) {
	    fifoWrite (Dome_Id, -6, "Dome is in Manual mode");
	    return (-1);
	}

	/* issue open */
	do {
	    writeCmd (open_cmd, sizeof(open_cmd)-1);
	    l = readBack (buf);
	} while (l < 7 || buf[5] != '0' || buf[6] != '0');
	return (0);
}

/* issue a low-level command to close the shutter */
static int
sraw_close()
{
	char buf[256];
	int l;

	/* check for manual mode */
	writeCmd (read_cmd, sizeof(read_cmd)-1);
	l = readBack (buf);
	if (l < 11 || ((buf[10]-'0') & 0x2)) {
	    fifoWrite (Dome_Id, -6, "Dome is in Manual mode");
	    return (-1);
	}

	do {
	    writeCmd (close_cmd, sizeof(close_cmd)-1);
	    l = readBack (buf);
	} while (l < 7 || buf[5] != '0' || buf[6] != '0');
	return (0);
}

/* issue command to rotate cw */
static void
draw_cw()
{
}

/* issue command to rotate ccw */
static void
draw_ccw()
{
}

/* issue dome stop */
static void
draw_stop()
{
}

/* read raw encoder */
static int
draw_readpos()
{
	return (0);
}

/* read raw home bit -- return 1 if low else 0 */
static int
draw_readhome()
{
	return (0);
}

/* save the current location as the home location */
static void
draw_markhome()
{
}

/* write a command to the roof controller */
static void
writeCmd (char cmd[], int n)
{
	Byte csbuf[256];
	Byte cs;

	if (!rttyfd)
	    rttyfd = openTTY();
	write (rttyfd, cmd, n);
	cs = csMake (cmd, n);
	csbuf[0] = (cs >> 4) + '0';
	csbuf[1] = (cs & 0xf) + '0';
	write (rttyfd, csbuf, 2);
	write (rttyfd, "*\r", 2);
}

/* read a response from the roof controller */
static int
readBack (char buf[])
{
	char *buf0 = buf;

	if (!rttyfd)
	    rttyfd = openTTY();
	do {
	    read (rttyfd, buf, 1);
	} while (*buf++ != '\n');	/* N.B. relies on ICRNL */

	return (buf - buf0);
}

/* compute the checksum of the n-byte msg[] */
static Byte
csMake (Byte msg[], int n)
{
	Byte cs = 0;
	int i;

	for (i = 0; i < n; i++)
	    cs ^= msg[i];
	return (cs);
}

/* open rttyname and set up connection, or exit */
static int
openTTY ()
{
	struct termios tio;
	int fd;

	fd = open (rttyname, O_RDWR|O_NONBLOCK);
	if (fd < 0) {
	    fifoWrite (Dome_Id, -1, "%s: %s\n",rttyname,strerror(errno));
	    printf ("%s: %s\n", rttyname, strerror(errno));
	    //exit(1);
            return(-1);	
	}
	if (fcntl (fd, F_SETFL, 0 /* no NONBLOCK now */ ) < 0) {
	    printf ("fctnl: %s: %s\n", rttyname, strerror(errno));
	    fifoWrite(Dome_Id, -1, "fctnl: %s: %s\n", rttyname, strerror(errno));
	    //exit(1);
            return(-1);	
	}

	/* set line characteristics */
	if (tcgetattr (fd, &tio) < 0) {
	    printf ("tcgetattr: %s: %s\n", rttyname, strerror(errno));
	    fifoWrite(Dome_Id,-1,"tcgetattr: %s: %s\n", rttyname, strerror(errno));
	    //exit(1);
	    return(-1);	
	}
	tio.c_iflag = ICRNL;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL;
	tio.c_lflag = 0;
	cfsetispeed (&tio, B9600);
	if (tcsetattr (fd, TCSANOW, &tio) < 0) {
	    printf ("tcsetattr: %s: %s\n", rttyname, strerror(errno));
	    fifoWrite(Dome_Id,-1,"tcsetattr: %s: %s\n", rttyname, strerror(errno));
	    //exit(1);
	    return(-1);	
	}

	return (fd);
}
#endif

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: dome.c,v $ $Date: 2003/04/15 20:48:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
