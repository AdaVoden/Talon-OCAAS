/* main dispatch and execution functions for the mount itself. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

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

#define MAXVL	521000	/* max vl command to pc39 */

/* handy loop "for each motor" we deal with here */
#define	FEM(p)		for ((p) = HMOT; (p) <= RMOT; (p)++)
#define	NMOT		(TEL_RM-TEL_HM+1)

/* the current activity, if any */
static void (*active_func) (int first, ...);

/* one of these... */
static void tel_poll(void);
static void tel_reset(int first);
static void tel_home(int first, ...);
static void tel_limits(int first, ...);
static void tel_radecep (int first, ...);
static void tel_radeceod (int first, ...);
static void tel_op (int first, ...);
static void tel_altaz (int first, ...);
static void tel_hadec (int first, ...);
static void tel_stop(int first, ...);
static void tel_jog (int first, ...);

/* helped along by these... */
static int dbformat (char *msg, Obj *op, double *drap, double *ddecp);
static void initCfg(void);
static void advance (double fpos, MotorInfo *mip);
static void hd2xyr (double ha, double dec, double *xp, double *yp, double *rp);
static void readRaw (void);
static void mkCook (void);
static void dummyTarg (void);
static void updateGuiding(void);
static void stopTel(int fast);
static int onTarget (MotorInfo **mipp);
static int atTarget (void);
static int trackObj (Obj *op);
static void findAxes (Obj *op, double dt, double *xp, double *yp, double *rp);
static int chkLimits (int wrapok, double *xp, double *yp, double *rp);
static void jogGuide (int first, char dircodes[]);
static void jogSlew (int first, char dircodes[]);
static int checkAxes(void);
static char *sayWhere (double alt, double az);

static double TRACKACC;		/* tracking accuracy, rads. 0 means 1 enc step*/

static double jogging_lmjd;	/* time of last jogging update */
static double FGUIDEVEL;	/* fine jogging motion rate, rads/sec */
static double CGUIDEVEL;	/* coarse jogging motion rate, rads/sec */
static double dguidevel;	/* current dec jogging velocity */
static double hguidevel;	/* current ha jogging velocity */

/* offsets to target object, if any */
static double r_offset;		/* delta ra to be added */
static double d_offset;		/* delta dec to be added */

/* called when we receive a message from the Tel fifo.
 * as well as regularly with !msg just to update things.
 */
/* ARGSUSED */
void
tel_msg (msg)
char *msg;
{
	double a, b, c;
	char jog_dir[8];
	Obj o;

	/* dispatch -- stop by default */

	if (!msg)
	    tel_poll();
	else if (strncasecmp (msg, "reset", 5) == 0)
	    tel_reset(1);
	else if (strncasecmp (msg, "home", 4) == 0)
	    tel_home(1, msg);
	else if (strncasecmp (msg, "limits", 6) == 0)
	    tel_limits(1, msg);
	else if (sscanf (msg, "RA:%lf Dec:%lf Epoch:%lf", &a, &b, &c) == 3)
	    tel_radecep (1, a, b, c);
	else if (sscanf (msg, "RA:%lf Dec:%lf", &a, &b) == 2)
	    tel_radeceod (1, a, b);
	else if (dbformat (msg, &o, &a, &b) == 0)
	    tel_op (1, &o, a, b);
	else if (sscanf (msg, "Alt:%lf Az:%lf", &a, &b) == 2)
	    tel_altaz (1, a, b);
	else if (sscanf (msg, "HA:%lf Dec:%lf", &a, &b) == 2)
	    tel_hadec (1, a, b);
	else if (sscanf (msg, "j%7[NSEWnsewH0]", jog_dir) == 1)
	    tel_jog (1, jog_dir);
	else
	    tel_stop(1);
}

/* no new messages.
 * goose the current objective, if any, else just update cooked position.
 */
static void
tel_poll()
{
	if (active_func)
	    (*active_func)(0);
	else {
	    /* idle -- just update */
	    readRaw();
	    mkCook();
	    dummyTarg();
	}
}

/* stop and reread config files */
static void
tel_reset(int first)
{
	MotorInfo *mip;

	initCfg();
	init_cfg();
	atf_cfg();

	FEM(mip) {
	    pc39_w ("a%c vl%d ac%d", mip->axis, MAXVELStp(mip), MAXACCStp(mip));
	}

	stopTel(0);
	active_func = NULL;
	fifoWrite (Tel_Id, 0, "Reset complete");
}

/* seek telescope axis home positions.. all or as per HDR */
static void 
tel_home(int first, ...)
{
	static int want[NMOT];
	static int nwant;
	MotorInfo *mip;
	int i;

	/* maintain cpos and raw for checkAxes() */
	readRaw();

	/* atf uses absolute encoders */
	if (atf_have())
	    goto atf;

	if (first) {
	    char *msg;

	    /* get the whole command */
	    va_list ap;
	    va_start (ap, first);
	    msg = va_arg (ap, char *);
	    va_end (ap);

	    /* start fresh */
	    stopTel(0);
	    memset ((void *)want, 0, sizeof(want));

	    /* find which axes to do, or all if none specified */
	    nwant = 0;
	    if (strchr (msg, 'H')) { nwant++; want[TEL_HM] = 1; }
	    if (strchr (msg, 'D')) { nwant++; want[TEL_DM] = 1; }
	    if (strchr (msg, 'R')) { nwant++; want[TEL_RM] = 1; }
	    if (!nwant) {
		nwant = 3;
		want[TEL_HM] = want[TEL_DM] = want[TEL_RM] = 1;
	    }

	    FEM (mip) {
		i = mip - &telstatshmp->minfo[0];
		if (want[i]) {
		    switch (axis_home (mip, Tel_Id, 1)) {
		    case -1:
			/* abort all axes if any fail */
			stopTel(1);
			active_func = NULL;
			return;
		    case 0:
			continue;
		    case 1:
			want[i] = 0;
			nwant--;
			break;
		    }
		}
	    }

	    /* if get here, set new state */
	    active_func = tel_home;
	    telstatshmp->telstate = TS_HOMING;
	    toTTS ("The telescope is seeking the home position.");
	}

	/* continue to seek home on each axis still not done */
	FEM (mip) {
	    i = mip - &telstatshmp->minfo[0];
	    if (want[i]) {
		switch (axis_home (mip, Tel_Id, 0)) {
		case -1:
		    /* abort all axes if any fail */
		    stopTel(1);
		    active_func = NULL;
		    return;
		case  0:
		    continue;
		case  1: 
		    fifoWrite (Tel_Id, 1, "Axis %c: home complete", mip->axis);
		    want[i] = 0;
		    nwant--;
		    break;
		}
	    }

	}

	/* really done when none left */
    atf:
	if (!nwant) {
	    telstatshmp->telstate = TS_STOPPED;
	    active_func = NULL;
	    fifoWrite (Tel_Id, 0, "Scope homing complete");
	    toTTS ("The telescope has found the home position.");
	}
}

/* find limit positions and H/D motor steps and signs.
 * N.B. set tax->h*lim as soon as we know TEL_HM limits.
 */
static void 
tel_limits(int first, ...)
{
	static int want[NMOT];
	static int nwant;
	MotorInfo *mip;
	int i;

	/* maintain cpos and raw */
	readRaw();

	if (first) {
	    char *msg;

	    /* get the whole command */
	    va_list ap;
	    va_start (ap, first);
	    msg = va_arg (ap, char *);
	    va_end (ap);

	    /* start fresh */
	    stopTel(0);
	    memset ((void *)want, 0, sizeof(want));

	    /* find which axes to do, or all if none specified */
	    nwant = 0;
	    if (strchr (msg, 'H')) { nwant++; want[TEL_HM] = 1; }
	    if (strchr (msg, 'D')) { nwant++; want[TEL_DM] = 1; }
	    if (strchr (msg, 'R')) { nwant++; want[TEL_RM] = 1; }
	    if (!nwant) {
		nwant = 3;
		want[TEL_HM] = want[TEL_DM] = want[TEL_RM] = 1;
	    }

	    FEM (mip) {
		i = mip - &telstatshmp->minfo[0];
		if (want[i]) {
		    switch (axis_limits (mip, Tel_Id, 1)) {
		    case -1:
			/* abort all axes if any fail */
			stopTel(1);
			active_func = NULL;
			return;
		    case 0:
			continue;
		    case 1:
			want[i] = 0;
			nwant--;
			break;
		    }
		}
	    }

	    /* new state */
	    active_func = tel_limits;
	    telstatshmp->telstate = TS_LIMITING;
	    toTTS ("The telescope is seeking the limit positions.");
	}

	/* continue to seek limits on each axis still not done */
	FEM (mip) {
	    i = mip - &telstatshmp->minfo[0];
	    if (want[i]) {
		switch (axis_limits (mip, Tel_Id, 0)) {
		case -1:
		    /* abort all axes if any fail */
		    stopTel(1);
		    active_func = NULL;
		    return;
		case  0:
		    continue;
		case  1: 
		    fifoWrite (Tel_Id, 2,"Axis %c: limits complete", mip->axis);
		    mip->cvel = 0;
		    want[i] = 0;
		    nwant--;
		    break;
		}
	    }

	}

	/* really done when none left */
	if (!nwant) {
	    stopTel(0);
	    initCfg();		/* read new limits */
	    active_func = NULL;
	    fifoWrite (Tel_Id, 0, "All Scope limits are complete.");
	    toTTS ("The telescope has found the limit positions.");

	    /* N.B. save TEL_HM limits in tax */
	    telstatshmp->tax.hneglim = HMOT->neglim;
	    telstatshmp->tax.hposlim = HMOT->poslim;
	}
}

/* handle tracking an astrometric position */
static void 
tel_radecep (int first, ...)
{
	static Obj o;

	if (first) {
	    Now *np = &telstatshmp->now;
	    Obj newo, *op = &newo;
	    double Mjd, ra, dec, ep;
	    va_list ap;

	    /* fetch values */
	    va_start (ap, first);
	    ra = va_arg (ap, double);
	    dec = va_arg (ap, double);
	    ep = va_arg (ap, double);
	    va_end (ap);

	    /* fill in op */
	    memset ((void *)op, 0, sizeof(*op));
	    op->f_RA = ra;
	    op->f_dec = dec;
	    year_mjd (ep, &Mjd);
	    op->f_epoch = Mjd;
	    op->o_type = FIXED;
	    strcpy (op->o_name, "<Anon>");

	    /* this is the new target */
	    o = *op;
	    active_func = tel_radecep;
	    telstatshmp->telstate = TS_HUNTING;
	    telstatshmp->jogging_ison = 0;
	    r_offset = d_offset = 0;
	    setSpeedDefaults(HMOT);
	    setSpeedDefaults(DMOT);
	    setSpeedDefaults(RMOT);

	    obj_cir (np, op);	/* just for sayWhere */
	    toTTS ("The telescope is slewing %s.",sayWhere(op->s_alt,op->s_az));
	}

	if (trackObj (&o) < 0)
	    active_func = NULL;
}

/* handle tracking an apparent position */
static void 
tel_radeceod (int first, ...)
{
	static Obj o;

	if (first) {
	    Now *np = &telstatshmp->now;
	    Obj newo, *op = &newo;
	    double ra, dec;
	    va_list ap;

	    /* fetch values */
	    va_start (ap, first);
	    ra = va_arg (ap, double);
	    dec = va_arg (ap, double);
	    va_end (ap);

	    /* fill in op */
	    ap_as (np, J2000, &ra, &dec);
	    memset ((void *)op, 0, sizeof(*op));
	    op->f_RA = ra;
	    op->f_dec = dec;
	    op->f_epoch = J2000;
	    op->o_type = FIXED;
	    strcpy (op->o_name, "<Anon>");

	    /* this is the new target */
	    o = *op;
	    active_func = tel_radeceod;
	    telstatshmp->telstate = TS_HUNTING;
	    telstatshmp->jogging_ison = 0;
	    r_offset = d_offset = 0;
	    setSpeedDefaults(HMOT);
	    setSpeedDefaults(DMOT);
	    setSpeedDefaults(RMOT);

	    obj_cir (np, op);	/* just for sayWhere */
	    toTTS ("The telescope is slewing %s.",sayWhere(op->s_alt,op->s_az));
	}

	if (trackObj (&o) < 0) 
	    active_func = NULL;
}

/* handle tracking an object */
static void 
tel_op (int first, ...)
{
	static Obj o;

	if (first) {
	    Now *np = &telstatshmp->now;
	    Obj *op;
	    va_list ap;

	    va_start (ap, first);
	    op = va_arg (ap, Obj *);
	    r_offset = va_arg (ap, double);
	    d_offset = va_arg (ap, double);
	    va_end (ap);

	    /* this is the new target */
	    o = *op;
	    active_func = tel_op;
	    telstatshmp->telstate = TS_HUNTING;
	    telstatshmp->jogging_ison = 0;
	    setSpeedDefaults(HMOT);
	    setSpeedDefaults(DMOT);
	    setSpeedDefaults(RMOT);

	    obj_cir (np, op);	/* just for sayWhere */
	    toTTS ("The telescope is slewing towards %s, %s.", op->o_name,
					    sayWhere (op->s_alt, op->s_az));
	}

	if (trackObj (&o) < 0)
	    active_func = NULL;
}

/* handle slewing to a horizon location */
static void 
tel_altaz (int first, ...)
{
	if (first) {
	    Now *np = &telstatshmp->now;
	    double alt, az;
	    double pa, ra, lst, ha, dec;
	    double x, y, r;
	    va_list ap;

	    /* gather target values */
	    va_start (ap, first);
	    alt = va_arg (ap, double);
	    az = va_arg (ap, double);
	    va_end (ap);

	    /* find target axis positions once */
	    aa_hadec (lat, alt, az, &ha, &dec);
	    telstatshmp->jogging_ison = 0;
	    r_offset = d_offset = 0;
	    hd2xyr (ha, dec, &x, &y, &r);
	    if (chkLimits (1, &x, &y, &r) < 0) {
		active_func = NULL;;
		return;	/* Tel_Id already informed */
	    }

	    /* set new state */
	    telstatshmp->telstate = TS_SLEWING;
	    active_func = tel_altaz;

	    /* set new raw destination */
	    HMOT->dpos = x;
	    DMOT->dpos = y;
	    RMOT->dpos = r;

	    /* and new cooked destination just for prying eyes */
	    telstatshmp->Dalt = alt;
	    telstatshmp->Daz = az;
	    telstatshmp->DAHA = ha;
	    telstatshmp->DADec = dec;
	    tel_hadec2PA (ha, dec, &telstatshmp->tax, lat, &pa);
	    telstatshmp->DPA = pa;
	    now_lst (np, &lst);
	    ra = hrrad(lst) - ha;
	    range (&ra, 2*PI);
	    telstatshmp->DARA = ra;
	    ap_as (np, J2000, &ra, &dec);
	    telstatshmp->DJ2kRA = ra;
	    telstatshmp->DJ2kDec = dec;

	    toTTS ("The telescope is slewing towards the %s, at %.0f degrees altitude.",
	    					cardDirLName(az), raddeg(alt));
	}

	/* stay up to date */
	readRaw();
	mkCook();

	/* the goal never moves */
	advance (HMOT->dpos, HMOT);
	advance (DMOT->dpos, DMOT);
	advance (RMOT->dpos, RMOT);

	if (checkAxes() < 0) {
	    stopTel(1);
	    active_func = NULL;
	}

	if (atTarget() == 0) {
	    stopTel(0);
	    fifoWrite (Tel_Id, 0, "Slew complete");
	    toTTS ("The telescope slew is complete");
	    active_func = NULL;
	}
}

/* handle slewing to an equatorial location */
static void 
tel_hadec (int first, ...)
{
	if (first) {
	    Now *np = &telstatshmp->now;
	    double alt, az, pa, ra, lst, ha, dec;
	    double x, y, r;
	    va_list ap;

	    /* gather params */
	    va_start (ap, first);
	    ha = va_arg (ap, double);
	    dec = va_arg (ap, double);
	    va_end (ap);

	    /* find target axis positions once */
	    telstatshmp->jogging_ison = 0;
	    r_offset = d_offset = 0;
	    hd2xyr (ha, dec, &x, &y, &r);
	    if (chkLimits (1, &x, &y, &r) < 0) {
		active_func = NULL;;
		return;	/* Tel_Id already informed */
	    }

	    /* set new state */
	    telstatshmp->telstate = TS_SLEWING;
	    active_func = tel_hadec;

	    /* set raw destination */
	    HMOT->dpos = x;
	    DMOT->dpos = y;
	    RMOT->dpos = r;

	    /* and cooked desination, just for enquiring minds */
	    telstatshmp->DAHA = ha;
	    telstatshmp->DADec = dec;
	    tel_hadec2PA (ha, dec, &telstatshmp->tax, lat, &pa);
	    hadec_aa (lat, ha, dec, &alt, &az);
	    telstatshmp->DPA = pa;
	    telstatshmp->Dalt = alt;
	    telstatshmp->Daz = az;
	    now_lst (np, &lst);
	    ra = hrrad(lst) - ha;
	    range (&ra, 2*PI);
	    telstatshmp->DARA = ra;
	    ap_as (np, J2000, &ra, &dec);
	    telstatshmp->DJ2kRA = ra;
	    telstatshmp->DJ2kDec = dec;

	    toTTS ("The telescope is slewing %s.", sayWhere (alt, az));
	}

	/* stay up to date */
	readRaw();
	mkCook();

	/* the goal never moves */
	advance (HMOT->dpos, HMOT);
	advance (DMOT->dpos, DMOT);
	advance (RMOT->dpos, RMOT);

	if (checkAxes() < 0) {
	    stopTel(1);
	    active_func = NULL;
	}

	if (atTarget() == 0) {
	    stopTel(0);
	    fifoWrite (Tel_Id, 0, "Slew complete");
	    toTTS ("The telescope slew is complete");
	    active_func = NULL;
	}
	    
}

/* politely stop all axes */
static void 
tel_stop (int first, ...)
{
	MotorInfo *mip;

	if (first) {
	    /* issue stops */
	    stopTel(0);
	    active_func = tel_stop;
	}

	/* wait for all to be stopped */
	FEM (mip) {
	    if (mip->have) {
		char buf[32];
		pc39_wr (buf, sizeof(buf), "a%c rv", mip->axis);
		if (atoi(buf))
		    return;
	    }
	}

	/* if get here, everything has stopped */
	telstatshmp->telstate = TS_STOPPED;
	active_func = NULL;
	fifoWrite (Tel_Id, 0, "Stop complete");
	toTTS ("The telescope is now stopped.");
	readRaw();
}

/* respond to a request for jogging */
static void
tel_jog (int first, ...)
{
	char *dircodes;

	if (first) {
	    va_list ap;
	    char d;
	    int i;

	    va_start (ap, first);
	    dircodes = va_arg (ap, char *);
	    va_end (ap);

	    for (i = 0; (d = dircodes[i]) != '\0'; i++) {
		switch (d) {
		case 'E': case 'W': case 'e': case 'w':
		    if (!HMOT->have) {
			fifoWrite (Tel_Id, -1, "No axis to jog %c", d);
			return;
		    }
		    break;
		case 'N': case 'S': case 'n': case 's':
		    if (!DMOT->have) {
			fifoWrite (Tel_Id, -1, "No axis to jog %c", d);
			return;
		    }
		    break;
		default:
		    /* H and 0 are ok in any case */
		    break;
		}
	    }
	} else
	    dircodes = "";

	if (telstatshmp->telstate == TS_TRACKING)
	    jogGuide (first, dircodes);
	else
	    jogSlew (first, dircodes);
}



/* aux support functions */

/* dig out a message with a db line in it.
 * may optionally be preceded by "dRA:x dDec:y #".
 * return 0 if ok, else -1.
 * N.B. always set dra/ddec.
 */
static int
dbformat (char *msg, Obj *op, double *drap, double *ddecp)
{
	if (sscanf (msg, "dRA:%lf dDec:%lf #", drap, ddecp) == 2)
	    msg = strchr (msg, '#') + 1;
	else
	    *drap = *ddecp = 0.0;

	return (db_crack_line (msg, op, NULL));
}

/* keep heading towards op + jog + sched.
 * we handle limit checks, maintain telstatus, check for loss of track.
 * return -1 when tracking is no longer possible, 0 when ok to keep trying.
 */
static int 
trackObj (Obj *objp)
{
	Now *np = &telstatshmp->now;
	Obj o, *op = &o;
	double ra, ha, dec, lst;
	MotorInfo *mip;
	double x, y, r;

	/* update current position info */
	readRaw();
	mkCook();

	/* check axes */
	if (checkAxes() < 0) {
	    stopTel(1);
	    return (-1);
	}

	/* update jog offset */
	if (telstatshmp->jogging_ison)
	    updateGuiding();

	/* find current desired topocentric apparent place and axes */
	*op = *objp;
	findAxes (op, 0.0, &x, &y, &r);
	if (chkLimits (1, &x, &y, &r) < 0) {
	    stopTel(0);
	    return (-1);
	}
	telstatshmp->Dalt = op->s_alt;
	telstatshmp->Daz = op->s_az;
	telstatshmp->DARA = ra = op->s_ra;
	telstatshmp->DADec = dec = op->s_dec;
	now_lst (np, &lst);
	ha = hrrad(lst) - ra;
	haRange (&ha);
	telstatshmp->DAHA = ha;
	ap_as (np, J2000, &ra, &dec);
	telstatshmp->DJ2kRA = ra;
	telstatshmp->DJ2kDec = dec;
	HMOT->dpos = x;
	DMOT->dpos = y;
	RMOT->dpos = r;

	/* check progress, revert to hunting if lose track */
	switch (telstatshmp->telstate) {
	case TS_HUNTING:
	    if (atTarget() == 0) {
		fifoWrite (Tel_Id, 3, "All axes have tracking lock");
		fifoWrite (Tel_Id, 0, "Now tracking");
		telstatshmp->telstate = TS_TRACKING;
		toTTS ("The telescope is now tracking.");
	    }
	    break;
	case TS_TRACKING:
	    if (!telstatshmp->jogging_ison && onTarget(&mip) < 0) {
		fifoWrite (Tel_Id, 4, "Axis %c lost tracking lock", mip->axis);
		toTTS ("The telescope has lost tracking lock.");
		telstatshmp->telstate = TS_HUNTING;
	    }
	    break;

	default:
	    break;
	}

	/* compute place at next clock tick */
	*op = *objp;
	findAxes (op, telstatshmp->dt, &x, &y, &r);
	if (chkLimits (1, &x, &y, &r) < 0) {
	    stopTel(0);
	    return (-1);
	}

	/* go there */
	advance (x, HMOT);
	advance (y, DMOT);
	advance (r, RMOT);

	return (0);
}

/* compute axes for *op at now+dt, including jog and schedule offsets.
 * return 0 if ok, -1 if exceeds limits
 * N.B. o_type of *op may be different upon return.
 */
static void
findAxes (Obj *op, double dt, double *xp, double *yp, double *rp)
{
	Now n = telstatshmp->now, *np = &n;
	double ha, dec;

	/* dt is in ms */
	mjd += dt/(1000.0*SPD);

	if (telstatshmp->jogging_ison || r_offset || d_offset) {
	    /* find offsets to op as a fixed object */
	    double ra, dec;

	    epoch = J2000;
	    obj_cir (np, op);
	    ra = op->s_ra;
	    dec = op->s_dec;

	    /* apply offsets */
	    if (telstatshmp->jogging_ison) {
		ra -= telstatshmp->jdha + hguidevel*dt/1000.;/*opposite signs!*/
		dec += telstatshmp->jddec + dguidevel*dt/1000.;
	    }
	    ra += r_offset;
	    dec += d_offset;

	    op->o_type = FIXED;
	    op->f_RA = ra;
	    op->f_dec = dec;
	    op->f_epoch = J2000;
	}

	epoch = EOD;
	obj_cir (np, op);
	aa_hadec (lat, op->s_alt, op->s_az, &ha, &dec);
	hd2xyr (ha, dec, xp, yp, rp);
}

/* issue jog to "future position" fpos.
 *
 * N.B.: when s_alt/az were floats finding velocity from difference of two
 * positions was too jerky. Now as doubles, differencing works because the
 * ephemeris is evidently *smooth*, even though it is still only accurate
 * to ~.1".
 */
static void
advance (double fpos, MotorInfo *mip)
{
	double dt = telstatshmp->dt/1000.;	/* update interval, secs */
	double ov;	/* object's vel, rads/sec */
	double iv;	/* ideal scope vel to hit obj in dt, rads/sec */
	double dv;	/* damped scope vel, rads/sec */
	double st;	/* actual step rate, usteps/sec */

	if (!mip->have)
	    return;

	ov = (fpos - mip->dpos)/dt;
	iv = (fpos - mip->cpos)/dt;
	if (telstatshmp->telstate == TS_TRACKING) {
	    dv = mip->trencwt*iv + (1-mip->trencwt)*ov;
	} else {
	    dv = mip->df*ov + (1-mip->df)*iv;
	}
	if (dv < -mip->maxvel) dv = -mip->maxvel;
	if (dv >  mip->maxvel) dv =  mip->maxvel;
	st = dv*mip->step*mip->sign/(2*PI);
	if (dv * mip->cvel < 0)
	    pc39_w ("a%c st jf%.3f", mip->axis, st); /*stop first if reversing*/
	else
	    pc39_w ("a%c jf%.3f", mip->axis, st);
	mip->cvel = dv;
}

/* convert an ha/dec to scope x/y/r, allowing for mesh corrections.
 * in many ways, this is the reverse of mkCook().
 */
static void
hd2xyr (double ha, double dec, double *xp, double *yp, double *rp)
{
	TelAxes *tap = &telstatshmp->tax;
	double mdha, mddec;
	double x, y, r;

	tel_mount_cor (ha, dec, &mdha, &mddec);
	ha += mdha;
	dec += mddec;
	hdRange (&ha, &dec);

	tel_hadec2xy (ha, dec, tap, &x, &y);
	tel_ideal2realxy (tap, &x, &y);
	if (RMOT->have) {
	    Now *np = &telstatshmp->now;
	    tel_hadec2PA (ha, dec, tap, lat, &r);
	    r += tap->R0 * RMOT->sign;
	} else
	    r = 0;

	*xp = x;
	*yp = y;
	*rp = r;
}

/* using the raw values, compute the astro position.
 * in many ways, this is the reverse of hd2xyz().
 */
static void
mkCook()
{
	Now *np = &telstatshmp->now;
	TelAxes *tap = &telstatshmp->tax;
	double lst, ra, ha, dec, alt, az;
	double mdha, mddec;
	double x, y, r;

	/* handy axis values */
	x = HMOT->cpos;
	y = DMOT->cpos;
	r = RMOT->cpos;

	/* back out non-ideal axes info */
	tel_realxy2ideal (tap, &x, &y);

	/* convert encoders to apparent ha/dec */
	tel_xy2hadec (x, y, tap, &ha, &dec);

	/* back out the mesh corrections */
	tel_mount_cor (ha, dec, &mdha, &mddec);
	telstatshmp->mdha = mdha;
	telstatshmp->mddec = mddec;
	ha -= mdha;
	dec -= mddec;
	hdRange (&ha, &dec);

	/* find horizon coords */
	hadec_aa (lat, ha, dec, &alt, &az);
	telstatshmp->Calt = alt;
	telstatshmp->Caz = az;

	/* find apparent equatorial coords */
	unrefract (pressure, temp, alt, &alt);
	aa_hadec (lat, alt, az, &ha, &dec);
	now_lst (np, &lst);
	lst = hrrad(lst);
	ra = lst - ha;
	range (&ra, 2*PI);
	telstatshmp->CARA = ra;
	telstatshmp->CAHA = ha;
	telstatshmp->CADec = dec;

	/* find J2000 astrometric equatorial coords */
	ap_as (np, J2000, &ra, &dec);
	telstatshmp->CJ2kRA = ra;
	telstatshmp->CJ2kDec = dec;

	/* find position angle */
	tel_hadec2PA (ha, dec, tap, lat, &r);
	telstatshmp->CPA = r;
}

/* read the raw values */
static void
readRaw ()
{
	MotorInfo *mip;

	if (atf_have()) {
	    int x, y;

	    atf_readraw (&x, &y);
	    mip = HMOT;
	    mip->raw = x;
	    mip->cpos = (2*PI) * mip->esign * x / mip->estep;
	    mip = DMOT;
	    mip->raw = y;
	    mip->cpos = (2*PI) * mip->esign * y / mip->estep;

	    return;
	}

	FEM(mip) {
	    char axis = mip->axis;
	    char buf[32];

	    if (!mip->have)
		continue;

	    if (nohw) {
		/* what a scope! */
		mip->cpos = mip->dpos;
		mip->raw= (int)floor(mip->esign*mip->estep*mip->cpos/(2*PI)+.5);
	    } else if (mip->haveenc) {
		double draw;
		int raw;

		/* just change by half-step if encoder changed by 1 */
		pc39_wr (buf, sizeof(buf), "a%c re", axis);
		raw = atoi(buf);
		draw = abs(raw - mip->raw)==1 ? (raw + mip->raw)/2.0 : raw;
		mip->raw = raw;
		mip->cpos = (2*PI) * mip->esign * draw / mip->estep;

	    } else {
		pc39_wr (buf, sizeof(buf), "a%c rp", axis);
		mip->raw = atoi(buf);
		mip->cpos = (2*PI) * mip->sign * mip->raw / mip->step;
	    }
	}
}

/* update jog offsets */
static void
updateGuiding()
{
	double dt;
	Now *np;

	if (!telstatshmp->jogging_ison)
	    return;

	np = &telstatshmp->now;
	dt = (mjd - jogging_lmjd)*SPD;	/* want secs */
	jogging_lmjd = mjd;

	telstatshmp->jdha += hguidevel*dt;
	telstatshmp->jddec += dguidevel*dt;
}

/* issue a stop to all telescope axes */
static void
stopTel(int fast)
{
	MotorInfo *mip;

	FEM (mip)
	    if (mip->have) {
		int ac = mip->step * (fast ? mip->slimacc : mip->maxacc)/(2*PI);

		pc39_w ("a%c ac%d st", mip->axis, ac);
		mip->cvel = 0;
		mip->limiting = 0;
		mip->homing = 0;
	    }

	telstatshmp->jogging_ison = 0;
	telstatshmp->telstate = TS_STOPPED;	/* well, soon anyway */
}

/* return 0 if all axes are within acceptable margin of desired, else -1 if
 * if any are out of range and set *mipp to offending axis.
 * N.B. use this only while tracking; use atTarget() when first acquiring.
 */
static int
onTarget(MotorInfo **mipp)
{
	MotorInfo *mip;

	FEM (mip) {
	    double trackacc;

	    if (!mip->have)
		continue;
		
	    /* tolerance: "0" means +-1 enc tick */
	    trackacc = TRACKACC == 0.0
			    ? (2*PI)/(mip->haveenc ? mip->estep : mip->step)
			    : TRACKACC;

	    if (delra (mip->cpos - mip->dpos) > trackacc) {
		*mipp = mip;
		return (-1);
	    }
	}

	/* all ok */
	return (0);
}

/* return 0 if all axes are within one encoder/motor count, else -1 if
 * if any are out of range.
 * N.B. use this when first acquiring; use onTarget() while tracking.
 */
static int
atTarget()
{
	static int non;
	MotorInfo *mip;

	FEM (mip) {
	    double trackacc;

	    if (!mip->have)
		continue;
		
	    trackacc = 2*(2*PI)/(mip->haveenc ? mip->estep : mip->step);

	    if (delra (mip->cpos - mip->dpos) > trackacc) {
		non = 0;
		return (-1);
	    }
	}

	/* if get here, all axes are within one count this time
	 * but still doesn't count until/unless it stays on for a few more.
	 */
	if (++non < 3)
	    return (-1);

	return (0);
}

/* check each canonical axis value for being beyond the hardware limit.
 * if wrapok, wrap the input values whole revolutions to accommodate limit.
 * if find any trouble, send failed message to Tel_Id and return -1.
 * else (all ok) return 0.
 */
static int
chkLimits (int wrapok, double *xp, double *yp, double *rp)
{
	double *valp[NMOT];
	MotorInfo *mip;
	char str[64];

	/* store so we can effectively access them via a mip */
	valp[TEL_HM] = xp;
	valp[TEL_DM] = yp;
	valp[TEL_RM] = rp;

	FEM (mip) {
	    double *vp = valp[mip - telstatshmp->minfo];
	    double v = *vp;

	    if (!mip->have)
		continue;

	    while (v <= mip->neglim) {
		if (!wrapok) {
		    fs_sexa (str, raddeg(v), 4, 3600);
		    fifoWrite (Tel_Id, -2, "Axis %c: %s hits negative limit",
								mip->axis, str);
		    return (-1);
		}
		v += 2*PI;
	    }

	    while (v >= mip->poslim) {
		if (!wrapok) {
		    fs_sexa (str, raddeg(v), 4, 3600);
		    fifoWrite (Tel_Id, -3, "Axis %c: %s hits positive limit",
								mip->axis, str);
		    return (-1);
		}
		v -= 2*PI;
	    }

	    /* double-check */
	    if (v <= mip->neglim || v >= mip->poslim) {
		fs_sexa (str, raddeg(v), 4, 3600);
		fifoWrite (Tel_Id, -4, "Axis %c: %s trapped within limits gap",
								mip->axis, str);
		return (-1);
	    }

	    /* pass back possibly updated */
	    *vp = v;
	}

	/* if get here, all ok */
	return (0);
}

/* set all desireds to currents */
static void
dummyTarg()
{
	HMOT->dpos = HMOT->cpos;
	DMOT->dpos = DMOT->cpos;
	RMOT->dpos = RMOT->cpos;

	telstatshmp->DJ2kRA = telstatshmp->CJ2kRA;
	telstatshmp->DJ2kDec = telstatshmp->CJ2kDec;
	telstatshmp->DARA = telstatshmp->CARA;
	telstatshmp->DADec = telstatshmp->CADec;
	telstatshmp->DAHA = telstatshmp->CAHA;
	telstatshmp->Dalt = telstatshmp->Calt;
	telstatshmp->Daz = telstatshmp->Caz;
	telstatshmp->DPA = telstatshmp->CPA;
}

/* called when get a j* jog command while TRACKING */
static void
jogGuide (int first, char *dircodes)
{
	if (first) {
	    int wanton = 1;
	    int wanthold = 0;
	    char d;

	    dguidevel = hguidevel = 0.0;

	    while ((d = *dircodes++) != '\0') {
		switch (d) {
		case 'N': dguidevel =  CGUIDEVEL; break;
		case 'n': dguidevel =  FGUIDEVEL; break;
		case 'S': dguidevel = -CGUIDEVEL; break;
		case 's': dguidevel = -FGUIDEVEL; break;
		case 'E': hguidevel = -CGUIDEVEL; break;
		case 'e': hguidevel = -FGUIDEVEL; break;
		case 'W': hguidevel =  CGUIDEVEL; break;
		case 'w': hguidevel =  FGUIDEVEL; break;
		case 'H': wanton = 0; break;	/* return to obj coords */
		case '0': wanthold = 1; break;	/* hold current position */
		default: tdlog ("Bogus jog direction: %c", d); return;
		}
	    }

	    /* be sure offsets are zeroed out and time inited when first on */
	    if (wanton) {
		if (!telstatshmp->jogging_ison) {
		    telstatshmp->jdha = telstatshmp->jddec = 0.0;
		    jogging_lmjd = telstatshmp->now.n_mjd;
		    telstatshmp->jogging_ison = 1;
		}
		if (wanthold) {
		    /* asign all error now to jogging offset */
		    telstatshmp->jdha += telstatshmp->CAHA- telstatshmp->DAHA;
		    telstatshmp->jddec+= telstatshmp->CADec- telstatshmp->DADec;
		}
	    } else {
		telstatshmp->jogging_ison = 0;
	    }
	}

	/* once setup here, trackObj() does the reset via updateGuiding() */
}

/* called when get a j* command while in any state other than TRACKING */
static void
jogSlew (int first, char *dircodes)
{
	/* maintain current info and slave the rotator */
	readRaw();
	mkCook();
	dummyTarg();
	/* TODO: r slave .. used everywhere?? */

	if (first) {
	    MotorInfo *mip;
	    char d;

	    /* not really used, but shm will show */
	    telstatshmp->jdha = telstatshmp->jddec = 0;

	    while ((d = *dircodes++) != '\0') {
		switch (d) {
		case 'N':
		    mip = DMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = mip->maxvel;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 5, "Paddle command up, fast");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case 'n':
		    mip = DMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = CGUIDEVEL;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 6, "Paddle command up, slow");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case 'S':
		    mip = DMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = -mip->maxvel;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 7, "Paddle command down, fast");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case 's':
		    mip = DMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = -CGUIDEVEL;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 8, "Paddle command down, slow");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case 'E':
		    mip = HMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = mip->maxvel;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 9, "Paddle command CCW, fast");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case 'e':
		    mip = HMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = CGUIDEVEL;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 10, "Paddle command CCW, slow");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case 'W':
		    mip = HMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = -mip->maxvel;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 11, "Paddle command CW, fast");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case 'w':
		    mip = HMOT;
		    setSpeedDefaults(mip);
		    mip->cvel = -CGUIDEVEL;
		    pc39_w ("a%c st wq jg%d", mip->axis, CVELStp(mip));
		    telstatshmp->telstate = TS_SLEWING;
		    fifoWrite (Tel_Id, 12, "Paddle command CW, fast");
		    active_func = tel_jog;
		    telstatshmp->jogging_ison = 1;
		    break;

		case '0':
		    stopTel(0);
		    fifoWrite (Tel_Id, 0, "Paddle command stop");
		    active_func = NULL;
		    return;	/* consider no other options */

		case 'H':	/* go home */
		    fifoWrite (Tel_Id, 13, "Paddle command Home");
		    mip = HMOT;
		    if (mip->have) {
			setSpeedDefaults(mip);
			pc39_w ("a%c vl%d ma0 go", mip->axis, MAXVELStp(mip));
			mip->cvel = mip->cpos > 0 ? -mip->maxvel : mip->maxvel;
		    }
		    mip = DMOT;
		    if (mip->have) {
			setSpeedDefaults(mip);
			pc39_w ("a%c vl%d ma0 go", mip->axis, MAXVELStp(mip));
			mip->cvel = mip->cpos > 0 ? -mip->maxvel : mip->maxvel;
		    }
		    break;

		default:
		    tdlog ("Bogus tel jog direction: %d", dircodes);
		    break;
		}
	    }
	}

	if (nohw) {
	    MotorInfo *mip;

	    mip = DMOT;
	    mip->dpos += mip->cvel*telstatshmp->dt/1000.;
	    mip = HMOT;
	    mip->dpos += mip->cvel*telstatshmp->dt/1000.;
	}

	/* this is under direct user control..
	 * about all we can do is watch for limits and
	 * stuck axes.
	 */

	if (checkAxes() < 0) {
	    active_func = NULL;
	    stopTel(1);
	    return;
	}
}

/* reread the config files -- exit if trouble */
static void
initCfg()
{
#define	NTDCFG	(sizeof(tdcfg)/sizeof(tdcfg[0]))
#define	NHCFG	(sizeof(hcfg)/sizeof(hcfg[0]))

	static double HMAXVEL, HMAXACC, HSLIMACC, HLIMMARG, HDAMP, HTRENCWT;
	static int HHAVE, HENCHOME, HPOSSIDE, HHOMELOW, HESTEP, HESIGN;
	static char HAXIS;

	static double DMAXVEL, DMAXACC, DSLIMACC, DLIMMARG, DDAMP, DTRENCWT;
	static int DHAVE, DENCHOME, DPOSSIDE, DHOMELOW, DESTEP, DESIGN;
	static char DAXIS;

	static double RMAXVEL, RMAXACC, RSLIMACC, RLIMMARG, RDAMP;
	static int RHAVE, RHASLIM, RPOSSIDE, RHOMELOW, RSTEP, RSIGN;
	static char RAXIS;

	static int POLL_PERIOD;
	static int GERMEQ;
	static int ZENFLIP;

	static CfgEntry tdcfg[] = {
	    {"HHAVE",		CFG_INT, &HHAVE},
	    {"HAXIS",		CFG_STR, &HAXIS, 1},
	    {"HENCHOME",	CFG_INT, &HENCHOME},
	    {"HHOMELOW",	CFG_INT, &HHOMELOW},
	    {"HPOSSIDE",	CFG_INT, &HPOSSIDE},
	    {"HESTEP",		CFG_INT, &HESTEP},
	    {"HESIGN",		CFG_INT, &HESIGN},
	    {"HLIMMARG",	CFG_DBL, &HLIMMARG},
	    {"HMAXVEL",		CFG_DBL, &HMAXVEL},
	    {"HMAXACC",		CFG_DBL, &HMAXACC},
	    {"HSLIMACC",	CFG_DBL, &HSLIMACC},
	    {"HTRENCWT",	CFG_DBL, &HTRENCWT},
	    {"HDAMP",		CFG_DBL, &HDAMP},

	    {"DHAVE",		CFG_INT, &DHAVE},
	    {"DAXIS",		CFG_STR, &DAXIS, 1},
	    {"DENCHOME",	CFG_INT, &DENCHOME},
	    {"DHOMELOW",	CFG_INT, &DHOMELOW},
	    {"DPOSSIDE",	CFG_INT, &DPOSSIDE},
	    {"DESTEP",		CFG_INT, &DESTEP},
	    {"DESIGN",		CFG_INT, &DESIGN},
	    {"DLIMMARG",	CFG_DBL, &DLIMMARG},
	    {"DMAXVEL",		CFG_DBL, &DMAXVEL},
	    {"DMAXACC",		CFG_DBL, &DMAXACC},
	    {"DSLIMACC",	CFG_DBL, &DSLIMACC},
	    {"DTRENCWT",	CFG_DBL, &DTRENCWT},
	    {"DDAMP",		CFG_DBL, &DDAMP},

	    {"RHAVE",		CFG_INT, &RHAVE},
	    {"RAXIS",		CFG_STR, &RAXIS, 1},
	    {"RHASLIM",		CFG_INT, &RHASLIM},
	    {"RHOMELOW",	CFG_INT, &RHOMELOW},
	    {"RPOSSIDE",	CFG_INT, &RPOSSIDE},
	    {"RSTEP",		CFG_INT, &RSTEP},
	    {"RSIGN",		CFG_INT, &RSIGN},
	    {"RLIMMARG",	CFG_DBL, &RLIMMARG},
	    {"RMAXVEL",		CFG_DBL, &RMAXVEL},
	    {"RMAXACC",		CFG_DBL, &RMAXACC},
	    {"RSLIMACC",	CFG_DBL, &RSLIMACC},
	    {"RDAMP",		CFG_DBL, &RDAMP},

	    {"POLL_PERIOD", 	CFG_INT, &POLL_PERIOD},
	    {"GERMEQ", 		CFG_INT, &GERMEQ},
	    {"ZENFLIP", 	CFG_INT, &ZENFLIP},
	    {"TRACKACC", 	CFG_DBL, &TRACKACC},
	    {"FGUIDEVEL", 	CFG_DBL, &FGUIDEVEL},
	    {"CGUIDEVEL", 	CFG_DBL, &CGUIDEVEL},
	};

 	static double HT, DT, XP, YC, NP, R0;
 	static double HPOSLIM, HNEGLIM, DPOSLIM, DNEGLIM, RNEGLIM, RPOSLIM;
	static int HSTEP, HSIGN, DSTEP, DSIGN;

	static CfgEntry hcfg[] = {
	    {"HT",		CFG_DBL, &HT},
	    {"DT",		CFG_DBL, &DT},
	    {"XP",		CFG_DBL, &XP},
	    {"YC",		CFG_DBL, &YC},
	    {"NP",		CFG_DBL, &NP},
	    {"R0",		CFG_DBL, &R0},

	    {"HPOSLIM",		CFG_DBL, &HPOSLIM},
	    {"HNEGLIM",		CFG_DBL, &HNEGLIM},
	    {"DPOSLIM",		CFG_DBL, &DPOSLIM},
	    {"DNEGLIM",		CFG_DBL, &DNEGLIM},
	    {"RNEGLIM",		CFG_DBL, &RNEGLIM},
	    {"RPOSLIM",		CFG_DBL, &RPOSLIM},

	    {"HSTEP",		CFG_INT, &HSTEP},
	    {"HSIGN",		CFG_INT, &HSIGN},
	    {"DSTEP",		CFG_INT, &DSTEP},
	    {"DSIGN",		CFG_INT, &DSIGN},
	};

	MotorInfo *mip;
	TelAxes *tap;
	int n;

	/* read in everything */
	n = readCfgFile (1, tdcfn, tdcfg, NTDCFG);
	if (n != NTDCFG) {
	    cfgFileError (tdcfn, n, (CfgPrFp)tdlog, tdcfg, NTDCFG);
	    die();
	}
	n = readCfgFile (1, hcfn, hcfg, NHCFG);
	if (n != NHCFG) {
	    cfgFileError (hcfn, n, (CfgPrFp)tdlog, hcfg, NHCFG);
	    die();
	}

	/* install H */

	if (HHAVE && (HSTEP * HMAXVEL / (2*PI) > MAXVL)) {
	    fprintf (stderr, "HSTEP*HMAXVEL must be < %g\n", (2*PI)*MAXVL);
	    die();
	}
	mip = &telstatshmp->minfo[TEL_HM];
	memset ((void *)mip, 0, sizeof(*mip));
	mip->axis = HAXIS;
	mip->have = HHAVE;
	mip->haveenc = 1;
	mip->enchome = HENCHOME;
	mip->havelim = 1;
	if (HPOSSIDE != 0 && HPOSSIDE != 1) {
	    fprintf (stderr, "HPOSSIDE must be 0 or 1\n");
	    die();
	}
	mip->posside = HPOSSIDE;
	if (HHOMELOW != 0 && HHOMELOW != 1) {
	    fprintf (stderr, "HHOMELOW must be 0 or 1\n");
	    die();
	}
	mip->homelow = HHOMELOW;
	mip->step = HSTEP;
	if (abs(HSIGN) != 1) {
	    fprintf (stderr, "HSIGN must be +-1\n");
	    die();
	}
	mip->sign = HSIGN;
	mip->estep = HESTEP;
	if (abs(HESIGN) != 1) {
	    fprintf (stderr, "HESIGN must be +-1\n");
	    die();
	}
	mip->esign = HESIGN;
	mip->limmarg = HLIMMARG;
	if (HMAXVEL <= 0) {
	    fprintf (stderr, "HMAXVEL must be > 0\n");
	    die();
	}
	mip->maxvel = HMAXVEL;
	mip->maxacc = HMAXACC;
	mip->slimacc = HSLIMACC;
	mip->poslim = HPOSLIM;
	mip->neglim = HNEGLIM;
	mip->trencwt = HTRENCWT;
	mip->df = HDAMP;

	/* install D */

	if (DHAVE && (DSTEP * DMAXVEL / (2*PI) > MAXVL)) {
	    fprintf (stderr, "DSTEP*DMAXVEL must be < %g\n", (2*PI)*MAXVL);
	    die();
	}
	mip = &telstatshmp->minfo[TEL_DM];
	memset ((void *)mip, 0, sizeof(*mip));
	mip->axis = DAXIS;
	mip->have = DHAVE;
	mip->haveenc = 1;
	mip->enchome = DENCHOME;
	mip->havelim = 1;
	if (DPOSSIDE != 0 && DPOSSIDE != 1) {
	    fprintf (stderr, "DPOSSIDE must be 0 or 1\n");
	    die();
	}
	mip->posside = DPOSSIDE;
	if (DHOMELOW != 0 && DHOMELOW != 1) {
	    fprintf (stderr, "DHOMELOW must be 0 or 1\n");
	    die();
	}
	mip->homelow = DHOMELOW;
	mip->step = DSTEP;
	if (abs(DSIGN) != 1) {
	    fprintf (stderr, "DSIGN must be +-1\n");
	    die();
	}
	mip->sign = DSIGN;
	mip->estep = DESTEP;
	if (abs(DESIGN) != 1) {
	    fprintf (stderr, "DESIGN must be +-1\n");
	    die();
	}
	mip->esign = DESIGN;
	mip->limmarg = DLIMMARG;
	if (DMAXVEL <= 0) {
	    fprintf (stderr, "DMAXVEL must be > 0\n");
	    die();
	}
	mip->maxvel = DMAXVEL;
	mip->maxacc = DMAXACC;
	mip->slimacc = DSLIMACC;
	mip->poslim = DPOSLIM;
	mip->neglim = DNEGLIM;
	mip->trencwt = DTRENCWT;
	mip->df = DDAMP;

	/* install R */

	if (RHAVE && (RSTEP * RMAXVEL / (2*PI) > MAXVL)) {
	    fprintf (stderr, "RSTEP*RMAXVEL must be < %g\n", (2*PI)*MAXVL);
	    die();
	}
	mip = &telstatshmp->minfo[TEL_RM];
	memset ((void *)mip, 0, sizeof(*mip));
	mip->axis = RAXIS;
	mip->have = RHAVE;
	mip->haveenc = 0;
	mip->enchome = 0;
	mip->havelim = RHASLIM;
	if (RPOSSIDE != 0 && RPOSSIDE != 1) {
	    fprintf (stderr, "RPOSSIDE must be 0 or 1\n");
	    die();
	}
	mip->posside = RPOSSIDE;
	if (RHOMELOW != 0 && RHOMELOW != 1) {
	    fprintf (stderr, "RHOMELOW must be 0 or 1\n");
	    die();
	}
	mip->homelow = RHOMELOW;
	mip->step = RSTEP;
	if (abs(RSIGN) != 1) {
	    fprintf (stderr, "RSIGN must be +-1\n");
	    die();
	}
	mip->sign = RSIGN;
	mip->estep = RSTEP;
	mip->esign = RSIGN;
	mip->limmarg = RLIMMARG;
	if (RMAXVEL <= 0) {
	    fprintf (stderr, "RMAXVEL must be > 0\n");
	    die();
	}
	mip->maxvel = RMAXVEL;
	mip->maxacc = RMAXACC;
	mip->slimacc = RSLIMACC;
	mip->poslim = RPOSLIM;
	mip->neglim = RNEGLIM;
	mip->trencwt = 0;
	mip->df = RDAMP;

	tap = &telstatshmp->tax;
	memset ((void *)tap, 0, sizeof(*tap));
	tap->GERMEQ = GERMEQ;
	tap->ZENFLIP = ZENFLIP;
	tap->HT = HT;
	tap->DT = DT;
	tap->XP = XP;
	tap->YC = YC;
	tap->NP = NP;
	tap->R0 = R0;

	tap->hneglim = telstatshmp->minfo[TEL_HM].neglim;
	tap->hposlim = telstatshmp->minfo[TEL_HM].poslim;

	telstatshmp->dt = POLL_PERIOD;

	/* re-read the mesh  file */
	init_mount_cor();

#undef	NTDCFG
#undef	NHCFG
}

/* check for stuck axes.
 * return 0 if all ok, else -1
 */
static int
checkAxes()
{
	MotorInfo *mip;
	char buf[1024];
	int nbad = 0;

	/* check all axes to learn more */
	FEM(mip) {
	    if (axisLimitCheck(mip, buf) < 0) {
		fifoWrite (Tel_Id, -8, "%s", buf);
		nbad++;
	    } else if (axisMotionCheck(mip, buf) < 0) {
		fifoWrite (Tel_Id, -9, "%s", buf);
		nbad++;
	    }
	}

	return (nbad > 0 ? -1 : 0);
}

/* return a static string to pronounce the location to reasonable precision */
static char *
sayWhere (double alt, double az)
{
	static char w[64];

	if (alt > degrad(72))
	    strcpy (w, "very hi");
	else if (alt > degrad(54))
	    strcpy (w, "hi");
	else if (alt > degrad(36))
	    strcpy (w, "half way up");
	else if (alt > degrad(18))
	    strcpy (w, "low");
	else
	    strcpy (w, "very low");

	strcat (w, ", in the ");
	strcat (w, cardDirLName(az));

	return (w);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: tel.c,v $ $Date: 2003/04/15 20:48:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
