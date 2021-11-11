/* code to actually run the telescope motors.
 *
 * (This is just the mount and field rotator; the focus and filter motors are
 * handled elsewhere)
 *
 * The original math derivation assumes the telescope's latitude axis (Dec or
 * Alt, ATCMB) values increases in the positive direction towards the N pole,
 * and the longitude axis (HA or Az) increases ccw as seen from above the
 * scope's N pole looking south (yes, like RA not HA). Whether the motors and
 * encoders really work that way is captured in the {H,D}[E]SIGN config file
 * params. Whenever an angular value appears in the code for axis, it is
 * the angle in radians from the home of that axis with the above sign.
 * This is referred to as the "canonical" directions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telstatshm.h"
#include "configfile.h"
#include "misc.h"
#include "teled.h"


extern TelStatShm *telstatshmp;

/* these are used to mark progress during homing sequence */
typedef enum {
    HS_IDLE = 0,		/* not active */
    HS_SEEKF,			/* slewing in "forward" direction */
    HS_SEEKB,			/* slewing in "backward" direction */
    HS_HOMEB,			/* going "backward" over home */
    HS_HOMEF,			/* crawling "forward" towards home */
    HS_DONE,			/* home found */
} HomeState;

/* state about an axis */
typedef struct {
    /* info from the config files */
    char axis;			/* pc39 axis code */
    int poshome;		/* 1 to go home positive, 0 for neg */
    int step, sign;		/* total motor steps in 1 rev, and sign */
    int maxvel;			/* max vel, usteps/sec */
    int maxsacc;		/* max slew acc, usteps/sec/sec */
    int maxtacc;		/* max tracking acc, usteps/sec/sec */
    int have;			/* 1 if installed, else 0 */
    int limmarg;		/* limit safety margin, motor steps */
    int estep, esign;		/* total encoder steps in 1 rev, and sign */
    double trencwt;		/* tracking encoder weight: 0-motor .. 1-enc */
    double poslim, neglim;	/* Pos and neg limit switch locations, rads */

    /* once set up, these point into telstatshmp */
    double *rate;		/* current motor rate */
    double *pos;		/* current position */
    double *targ;		/* desired future position */
    double *pref;		/* desired current position */
    int *rawpos;		/* raw position */

    /* misc */
    HomeState hs;		/* homing progress */
} AxisInfo;
typedef enum {
    AX_H = 0,			/* longitudinal axis: HA or Az */
    AX_D,			/* latitudinal axis: Dec or Alt */
    AX_R,			/* field rotator */
    AX_N
} Axes;
static AxisInfo axinfo[AX_N];

/* handy direct access */
static AxisInfo *haip = &axinfo[AX_H];
static AxisInfo *daip = &axinfo[AX_D];
static AxisInfo *raip = &axinfo[AX_R];

#define	HOMEF(aip)	((aip)->poshome ? 'm' : 'r')	/* home cmd forwards */
#define	HOMEB(aip)	((aip)->poshome ? 'r' : 'm')	/* home cmd backwards */

/* names of config files */
static char tdcfg[] = "archive/config/telescoped.cfg";
static char hcfg[] = "archive/config/home.cfg";

static double ACQERR;			/* hunting error, rads */

static void tel_read_cfg(void);
static int tel_objxyr (Now *np, Obj *op, double *xp, double *yp, double *rp);;
static void tel_cooked (void);
static int tel_readaxis (AxisInfo *aip);
static int tel_XstartFindingLimits (AxisInfo *aip);
static int tel_XFindingLimits (AxisInfo *aip, char *posname, char *negname);
static int tel_Xstarthome(AxisInfo *aip);
static int tel_Xchkhome(AxisInfo *aip);

/* reset everthing about the mount short of finding home.
 * for that, use tel_starthome() and tel_chkhome().
 * return 0 if all ok, else -1.
 */
int 
tel_reset(int hwtoo)
{
	static char fmt[] = "a%c lf ac%d vl%d st";
	Obj *op;
	int n = 0;
	int s;

	/* (re)read config and mesh files */
	tel_read_cfg();
	init_mount_cor();

	/* set up pointers from telstatshmp into axinfo */
	haip->rate = &telstatshmp->hrate;
	haip->pos  = &telstatshmp->hpos;
	haip->targ = &telstatshmp->htarg;
	haip->pref = &telstatshmp->hpref;
	haip->rawpos  = &telstatshmp->hrpos;

	daip->rate = &telstatshmp->drate;
	daip->pos  = &telstatshmp->dpos;
	daip->targ = &telstatshmp->dtarg;
	daip->pref = &telstatshmp->dpref;
	daip->rawpos  = &telstatshmp->drpos;

	raip->rate = &telstatshmp->rrate;
	raip->pos  = &telstatshmp->rpos;
	raip->targ = &telstatshmp->rtarg;
	raip->pref = &telstatshmp->rpref;
	raip->rawpos  = &telstatshmp->rrpos;

	/* reset controller too if hwtoo -- this blows away all position info */
	if (hwtoo) {
	    if (pc39_reset() < 0)
		return (-1);
	    (void) tel_sane_progress (1);
	}

	/* stop all axes, set up for cosine acc.
	 * N.B. if use linear ac, change slewTime().
	 */
	s = pc39_w (" sa cn ");
	if (s < 0)
	    n++;

	/* limits on, init basic params, stop */
	s = pc39_w (fmt, haip->axis, haip->maxsacc, haip->maxvel);
	if (s < 0)
	    n++;
	s = pc39_w (fmt, daip->axis, daip->maxsacc, daip->maxvel);
	if (s < 0)
	    n++;
	if (raip->have) {
	    s = pc39_w (fmt, raip->axis, raip->maxsacc, raip->maxvel);
	    if (s < 0)
		n++;
	}

	if (n > 0) {
	    tdlog ("Telescope: Error resetting\n");
	    return (-1);
	}

	/* redo state */
	telstatshmp->telstatus = TS_STOPPED;
	telstatshmp->odra = telstatshmp->oddec = 0.0;
	telstatshmp->odalt = telstatshmp->odaz = 0.0;
	telstatshmp->odh = telstatshmp->odd = 0.0;
	op = &telstatshmp->obj;
	op->o_type = FIXED;
	op->f_RA = 0.0;
	op->f_dec = 0.0;
	op->f_epoch = J2000;
	strcpy (op->o_name, "Init");
	if (tel_readraw() < 0)
	    return (-1);

	return (0);
}

/* stop the telescope axes */
void
tel_stop()
{
	(void) pc39_w ("a%c st a%c st a%c st", haip->axis, daip->axis,
								    raip->axis);
}

/* panic-stop the whole system */
void
tel_allstop()
{
	/* should probably set telstatshmp->*rate, but sometimes it's nice
	 * to leave them for post mortem analysis :-)
	 */

	(void) pc39_w ("sa");
	stopDome();

	/* TODO:stop the shutter, but it's a dumb timed thing now */
}

/* read the encoders (or motors, ATCMB) and save positions, velocities
 * and compute the corresponding effective astrometric values in telstatshmp.
 * return 0 if ok, else -1.
 */
int
tel_readraw()
{
	unsigned char bits[12];

	/* read HA and Dec from absolute encoders.
	 * set up 8255s as input for ha/dec encoders, output for TG bit
	 */
	dio96_ctrl (0, 0x9b);    /* all lines input on 1st chip */
	dio96_ctrl (1, 0x93);    /* all lines input on 2nd chip but C-hi */

	/* strobe the TG encoders, delay, read */
	dio96_clrbit (44);  /* clear the TG bit */
	dio96_setbit (44);  /* set the TG bit */
	usleep (10000);     /* delay, in us */
	dio96_getallbits (bits);/* read all encoders */
	*haip->rawpos = ((unsigned)bits[1]<<8) | (unsigned)bits[0];
	*daip->rawpos = ((unsigned)bits[4]<<8) | (unsigned)bits[3];

	/* we want signed dec values */
	if (*daip->rawpos > 32768)
	    *daip->rawpos -= 65536;

	/* read the axes */
	if (tel_readaxis (haip) < 0)
	    return (-1);
	if (tel_readaxis (daip) < 0)
	    return (-1);

	/* update the cooked values */
	tel_cooked ();

	return (0);
}

/* convert the given apparent RA/Dec to astrometric precessed to Mjd IN PLACE.
 * we have no un-abberation etc so to find the correction: assume
 * *rap and *decp are astrometric@EOD, convert to apparent and back out
 * the difference; then precess to Mjd.
 */
void
tel_ap2as (double Mjd, double *rap, double *decp)
{
	Now *np = &telstatshmp->now;
	Obj o;
	Now n;

	o.o_type = FIXED;
	o.f_RA = *rap;
	o.f_dec = *decp;
	o.f_epoch = mjd;
	n = *np;
	n.n_epoch = EOD;
	obj_cir (&n, &o);
	*rap -= o.s_ra - *rap;
	range (rap, 2*PI);
	*decp -= o.s_dec - *decp;
	decRange (decp);
	precess (mjd, Mjd, rap, decp);
}

/* start finding the limits of the ha, dec and rotation axes.
 * N.B. we assume homing has already been performed.
 * return -1 if trouble, else 0.
 */
int
tel_startFindingLimits()
{
	if (tel_XstartFindingLimits (haip) < 0)
	    return (-1);
	if (tel_XstartFindingLimits (daip) < 0)
	    return (-1);
	if (tel_XstartFindingLimits (raip) < 0)
	    return (-1);
	return (0);
}

/* keep finding limits, writing new values to home.cfg when know all.
 * return 0 when all finished, else -1.
 */
int
tel_FindingLimits()
{
	int ndone = 0;

	ndone += tel_XFindingLimits (haip, "HPOSLIM", "HNEGLIM");
	ndone += tel_XFindingLimits (daip, "DPOSLIM", "DNEGLIM");
	ndone += tel_XFindingLimits (raip, "RPOSLIM", "RNEGLIM");

	return (ndone == 3 ? 0 : -1);
}

/* no home so just say ok.
 * return 0 if all ok and underway, else -1.
 */
int
tel_starthome()
{
	int s;

	/* (re)read config and mesh files */
	tel_read_cfg();
	init_mount_cor();

	/* start each axis heading home */
	s = tel_Xstarthome (haip) + tel_Xstarthome (daip)
					   + tel_Xstarthome (raip);

	return (s ? -1 : 0);
}


/* check all the mount motors for progress finding home.
 * return -1 if any errors, 0 if all making progress, 1 if all found home.
 * N.B. we assume tel_starthome() has already been called successfully.
 */
int
tel_chkhome()
{
	int sh, sd, sr;

	if ((sh = tel_Xchkhome (haip)) < 0)
	    return (-1);
	if ((sd = tel_Xchkhome (daip)) < 0)
	    return (-1);
	if ((sr = tel_Xchkhome (raip)) < 0)
	    return (-1);

	return (sh + sd + sr == 3 ? 1 : 0);
}

/* issue a fresh move to the (future) target position for the given axis.
 * return 1 if not installed or already within ACQERR, else 0 if underway.
 */
static int
slewToTarget (AxisInfo *aip)
{
	double diff;
	int r;

	/* done if not installed or stopped and within ACQERR */
	if (!aip->have)
	    return (1);
	if (*aip->rate != 0.0)
	    return (0);
	if (fabs (*aip->pref - *aip->pos) <= ACQERR)
	    return (1);

	/* issue move from here to target */
	diff = *aip->targ - *aip->pos;
	r  = (int)floor(diff/(2*PI)*aip->step*aip->sign + 0.5);

	(void) pc39_w ("a%c ac%d vl%d st mr%d go", aip->axis, aip->maxsacc,
							    aip->maxvel, r);
	return (0);
}

/* start or check slewing.
 * if first, compute obj and set targets, else just use them.
 * don't claim success until we've nailed each axis to ACQERR.
 * return: -1; slew would hit limit; 0: moving ok; 1: on target.
 */
int
tel_slew (int first)
{
	int ndone;

	/* first time only, compute obj and set target values */
	if (first) {
	    Now *np = &telstatshmp->now;
	    Obj *op = &telstatshmp->obj;

	    /* stop to enable updates on subsequent calls */
	    tel_stop();

	    /* compute axis targets now _once only_ */
	    if (tel_objxyr (np, op, &telstatshmp->htarg,
				&telstatshmp->dtarg, &telstatshmp->rtarg) < 0)
		return (-1);

	    /* that's where we wish we were now too */
	    telstatshmp->hpref = telstatshmp->htarg;
	    telstatshmp->dpref = telstatshmp->dtarg;
	    telstatshmp->rpref = telstatshmp->rtarg;
	}

	/* keep making progress towards the targets */
	ndone = slewToTarget(haip) + slewToTarget(daip) + slewToTarget(raip);

	/* on target if all axes are done */
	return (ndone == 3 ? 1 : 0);
}

/* issue a velocity which puts the motor at targ dtms.
 * use a weighted average between using pref and pos based on trencwt.
 * we also check for direction change and clamp to safe velocties, as needed.
 */
static void
trackToTarget (AxisInfo *aip, int dtms)
{
	double vm, ve, v;

	if (!aip->have)
	    return;

	/* compute based on encoder */
	ve = *aip->targ - *aip->pos;

	/* compute based on theoretical current */
	vm = *aip->targ - *aip->pref;

	/* find weighted sum based on trencwt */
	v = aip->trencwt*ve + (1.0 - aip->trencwt)*vm;

	/* find velocity, correct for motor sign */
	v *= aip->step*aip->sign*(1000.0/(2*PI))/dtms;

	/* issue new velocity, after clamping to safe limits.
	 * also, seem to need st if changing directions.
	 */
	if (v >  aip->maxvel) v =  aip->maxvel;
	if (v < -aip->maxvel) v = -aip->maxvel;

	if (*aip->rate * v*aip->sign < 0)
	    (void) pc39_w ("a%c st ac%d jf%.2f ", aip->axis, aip->maxtacc, v);
	else
	    (void) pc39_w ("a%c ac%d jf%.2f ", aip->axis, aip->maxtacc, v);
}

/* compute time to slew from pos to targ, in seconds.
 * N.B. this assumes cosine acceleration. if use linear, change (PI-1)
 *   to simply 1.
 */
static double
slewTime (AxisInfo *aip, double targ)
{
	double dt;
	
	if (!aip->have)
	    return (0.0);

	dt = fabs(targ - *aip->pos)/(2*PI)*aip->step/aip->maxvel
				    + (PI-1)*(double)aip->maxvel/aip->maxsacc;

	return (dt);
}

/* search for current object by first slewing, then creeping up by tracking.
 * return: -1 slew would hit limit; 0: moving ok; 1: on target.
 */
int
tel_hunt (int dtms, int first)
{
	Now *np = &telstatshmp->now;	/* handy */
	Obj *op = &telstatshmp->obj;	/* handy */
	Now n = telstatshmp->now;	/* make copy so we can change here */
	Obj o = telstatshmp->obj;	/* make copy so we can change here */
	int s;

	/* update the ephemeris now */
	if (tel_objxyr (np, op, &telstatshmp->hpref, &telstatshmp->dpref,
						    &telstatshmp->rpref) < 0)
	    return (-1);

	if (first) {
	    /* to help the first slew, account for time to slew */
	    double th, td, tr, tmax;

	    /* stop for slew */
	    tel_stop();

	    th = slewTime (haip, telstatshmp->hpref);
	    td = slewTime (daip, telstatshmp->dpref);
	    tr = slewTime (raip, telstatshmp->rpref);
	    tmax = th > td ? th : td;
	    tmax = tmax > tr ? tmax : tr;

	    dtms += tmax*1000.0;
	}

	n.n_mjd += dtms/1000./3600.0/24.0;		/* want days */
	if (tel_objxyr (&n, &o, &telstatshmp->htarg, &telstatshmp->dtarg,
						&telstatshmp->rtarg) < 0)
	    return (-1);

	/* we've already computed the target here for slewing */
	s = tel_slew(0);

	/* if dead on, start tracking immediately */
	if (s == 1) {
	    if (tel_track (dtms) < 0)
		s = -1;
	}
	return (s);
}

/* issue velocity commands to maintain track on the current object.
 * this uses the motor steps, not the encoder.
 * return 0 if ok, else -1 if encounter a limit.
 */
int
tel_track (int dtms)
{
	Now *np = &telstatshmp->now;	/* handy */
	Obj *op = &telstatshmp->obj;	/* handy */
	Now n = telstatshmp->now;	/* copy so we can change to dtms */
	Obj o = telstatshmp->obj;	/* copy so we can change to dtms */

	/* compute ephemeris of object now */
	if (tel_objxyr (np, op, &telstatshmp->hpref, &telstatshmp->dpref,
						    &telstatshmp->rpref) < 0)
	    return (-1);

	/* compute ephemeris of object @ now+dtms */
	n.n_mjd += dtms/1000./3600./24.;	/* ms -> days */
	if (tel_objxyr (&n, &o, &telstatshmp->htarg, &telstatshmp->dtarg,
						    &telstatshmp->rtarg) < 0)
	    return (-1);

	/* issue velocities */
	trackToTarget (haip, dtms);
	trackToTarget (daip, dtms);
	trackToTarget (raip, dtms);

	return (0);
}

/* no home so just say ok.
 * return 0 if ok and underway, else -1.
 * N.B. we thoughtfully return 0 if !aip->have.
 */
static int
tel_Xstarthome(AxisInfo *aip)
{
	/* pretend ok if doesn't exist */
	if (!aip->have)
	    return (0);

	/* just set for viewer's sake */
	*aip->pref = *aip->targ = 0.0;
	aip->hs = HS_SEEKF;

	return (0);
}

/* check the given axis for homing progress.
 * no limits so just say ok.
 * return -1 if error, 0 if making progress, 1 if found home.
 * N.B. we assume tel_Xstarthome() has already been called successfully.
 * N.B. we thoughtfully return 1 if !aip->have.
 */
static int
tel_Xchkhome(AxisInfo *aip)
{
	/* pretend to be home if doesn't exist */
	if (!aip->have)
	    return (1);

	*aip->rate = 0.0;
	aip->hs = HS_DONE;
	return (1);
}

/* start finding the limits of the given axis.
 * no limits so just say ok.
 * return 0 if ok, else -1
 */
static int
tel_XstartFindingLimits (AxisInfo *aip)
{
	return (0);
}

/* keep seeking limits on the given axis, writing new values to home.cfg when
 * discovered.
 * no limits so just no.
 * return 1 when finished with both directions, 0 if still working.
 */
static int
tel_XFindingLimits (AxisInfo *aip, char *posname, char *negname)
{
	/* always "done" if axis is not enabled */
	if (!aip->have)
	    return (1);

	/* wait until hit limit as evidenced by no motion */
	*aip->rate = 0.0;

	return (1);
}

/* compute pos from rawpos; read velocity.
 * return -1 if trouble, 0 if ok.
 */
static int
tel_readaxis (AxisInfo *aip)
{
	double pos, rate;
	char back[64];
	int rawp, rawr;

	if (!aip->have)
	    return (0);

	rawp = *aip->rawpos;
	pos = (2*PI) * rawp / aip->estep * aip->esign;

	if (pc39_wr (back, sizeof(back), "a%c rv", aip->axis) < 0) {
	    tdlog ("Tel %c: rv error", aip->axis);
	    return(-1);
	}
	rawr = atoi(back);
	rate = (2*PI) * rawr / aip->step;	/* don't know sign yet */

	/* pc39 rv always returns absolute value of rate -- decide sign
	 * based on how sign of current position compares to previous
	 */
	if (pos < *aip->pos)
	    rate = -rate;
	*aip->rate = rate;
	*aip->pos = pos;

	return (0);
}

/* check the given candidate target against the limits for the given axis.
 * return 0 if ok, else -1.
 */
static int
chkLimit (AxisInfo *aip, double *tp)
{
	static int wrapok = 1;	/* TODO remove if really always wrap */
	double t = *tp;	/* handy */

	/* always ok if not installed */
	if (!aip->have)
	    return (0);

	/* test and possibly accommodate each limit */
	while (t <= aip->neglim) {
	    if (!wrapok) {
		tdlog("%c axis request beyond neg limit: %g < %g", aip->axis,
							    t, aip->neglim);
		return (-1);
	    }
	    t += 2*PI;
	}
	while (t >= aip->poslim) {
	    if (!wrapok) {
		tdlog("%c axis request beyond pos limit: %g > %g", aip->axis,
							    t, aip->poslim);
		return (-1);
	    }
	    t -= 2*PI;
	}

	/* double-check */
	if (t <= aip->neglim || t >= aip->poslim) {
	    tdlog ("%c axis still beyond limits: %g %g .. %g", aip->axis,
						t, aip->neglim, aip->poslim);
	    return (-1);
	}

	/* ok */
	*tp = t;
	return (0);
}

/* given an Obj current as of Now find the axis values there then, in rads.
 * include all desired offsets and corrections.
 * return -1 if any values would hit a limit, else 0 if all ok.
 */
static int
tel_objxyr (Now *np, Obj *op, double *xp, double *yp, double *rp)
{
	double mdha, mddec;
	double alt, az, ha, dec;
	double x, y, r;

	/* find topocentric refracted apparent place, with alt/az offsets */
	epoch = EOD;
	(void) obj_cir (np, op);
	alt = op->s_alt + telstatshmp->odalt;
	az = op->s_az + telstatshmp->odaz;
	if (alt > PI/2) {
	    az += PI;
	    alt = PI - alt;
	} else if (alt < -PI/2 /*!*/) {
	    az += PI;
	    alt = -PI - alt;
	}
	range (&az, 2*PI);
	aa_hadec (lat, alt, az, &ha, &dec);

	/* apply ra/dec offsets and mesh corrections */
	ha -= telstatshmp->odra;
	haRange (&ha);
	dec += telstatshmp->oddec;
	decRange (&dec);
	tel_mount_cor (ha, dec, &mdha, &mddec);
	ha += mdha;
	haRange (&ha);
	dec += mddec;
	decRange (&dec);

	/* convert to generic x/y/r angles from homes */
	tel_hadec2xy (ha, dec, &telstatshmp->tax, &x, &y);

	/* correct for non-perp axes */
	tel_ideal2nonp (&telstatshmp->tax, &x, &y);

	/* apply raw offsets with proper sign */
	x += telstatshmp->odh*haip->sign;
	y += telstatshmp->odd*daip->sign;

	/* check limits, and wrap */
	if (chkLimit (haip, &x) < 0 || chkLimit (daip, &y) < 0)
	    return (-1);

	/* and finally, find field rotator angle if have one */
	if (raip->have) {
	    tel_hadec2PA(ha, dec, &telstatshmp->tax, telstatshmp->now.n_lat,&r);
	    r += telstatshmp->tax.R0*raip->sign;
	    if (chkLimit (raip, &r) < 0)
		return (-1);
	} else
	    r = 0.0;

	/* pass back up */
	*xp = x;
	*yp = y;
	*rp = r;
	return (0);
}

/* update all the cooked positions in telstatshmp:
 * EODRA, EODHA, EODDec, J2000RA, J2000Dec, alt, az.
 * these are where the scope is really pointing now on the sky.
 */
static void
tel_cooked ()
{
	Now *np = &telstatshmp->now;
	double lst, ra, ha, dec, alt, az;
	double mdha, mddec;
	double x, y, r;

	/* handy axis values */
	x = telstatshmp->hpos;
	y = telstatshmp->dpos;
	r = telstatshmp->rpos;

	/* back out non-perp axes */
	tel_nonp2ideal (&telstatshmp->tax, &x, &y);

	/* convert encoders to apparent ha/dec */
	tel_xy2hadec (x, y, &telstatshmp->tax, &ha, &dec);

	/* back out the mesh corrections */
	tel_mount_cor (ha, dec, &mdha, &mddec);
	ha -= mdha;
	haRange (&ha);
	dec -= mddec;
	decRange (&dec);

	/* find horizon coords */
	hadec_aa (lat, ha, dec, &alt, &az);
	telstatshmp->alt = alt;
	telstatshmp->az = az;

	/* find apparent equatorial coords */
	unrefract (pressure, temp, alt, &alt);
	aa_hadec (lat, alt, az, &ha, &dec);
	now_lst (np, &lst);
	lst = hrrad(lst);
	ra = lst - ha;
	range (&ra, 2*PI);
	telstatshmp->EODRA = ra;
	telstatshmp->EODHA = ha;
	telstatshmp->EODDec = dec;

	/* find J2000 astrometric equatorial coords */
	tel_ap2as (J2000, &ra, &dec);
	telstatshmp->J2000RA = ra;
	telstatshmp->J2000Dec = dec;
}

/* (re)read the config file params.
 * exit if trouble.
 */
static void
tel_read_cfg()
{
	AxisInfo *aip;
	char value[10];
	double tmp;

	setConfigFile (tdcfg, 1);

	/* net acquisition error */
	readDVal ("ACQERR", &ACQERR);

	/* special flipping for german equatorials */
	readIVal ("GERMEQ", &telstatshmp->tax.GERMEQ);

	/* zenith flip */
	readIVal ("ZENFLIP", &telstatshmp->tax.ZENFLIP);

	/* X axis */
	aip = haip;
	aip->have = 1;
	readIVal ("HSTEP", &aip->step);
	readDVal ("HMAXVEL", &tmp);
	aip->maxvel = aip->step*tmp/(2*PI) + 0.5;
	readDVal ("HMAXSACC", &tmp);
	aip->maxsacc = aip->step*tmp/(2*PI) + 0.5;
	readDVal ("HMAXTACC", &tmp);
	aip->maxtacc = aip->step*tmp/(2*PI) + 0.5;
	readIVal ("HSIGN", &aip->sign);
	if (abs(aip->sign) != 1) {
	    tdlog ("HSIGN must be 1 or -1: %d", aip->sign);
	    exit(1);
	}
	readIVal ("HESIGN", &aip->esign);
	if (abs(aip->esign) != 1) {
	    tdlog ("HESIGN must be 1 or -1: %d", aip->esign);
	    exit(1);
	}
	readIVal ("HESTEP", &aip->estep);
	readDVal ("HTRENCWT", &aip->trencwt);
	if (aip->trencwt < 0 || aip->trencwt > 1) {
	    tdlog ("HTRENCWT must be 0..1: %d", aip->trencwt);
	    exit(1);
	}
	readDVal ("HLIMMARG", &tmp);
	aip->limmarg = tmp/(2*PI)*aip->step;
	readIVal ("HHOMEDIR", &aip->poshome);
	if (aip->poshome != 1 && aip->poshome != 0) {
	    tdlog ("HHOMEDIR must be 1 or 0: %d", aip->poshome);
	    exit(1);
	}
	if (readCFG ("HAXIS", value, sizeof(value)) < 0) {
	    tdlog ("No HAXIS in %s", tdcfg);
	    exit(1);
	}
	aip->axis = value[0];

	/* Y axis */
	aip = daip;
	aip->have = 1;
	readIVal ("DSTEP", &aip->step);
	readDVal ("DMAXVEL", &tmp);
	aip->maxvel = aip->step*tmp/(2*PI) + 0.5;
	readDVal ("DMAXSACC", &tmp);
	aip->maxsacc = aip->step*tmp/(2*PI) + 0.5;
	readDVal ("DMAXTACC", &tmp);
	aip->maxtacc = aip->step*tmp/(2*PI) + 0.5;
	readIVal ("DSIGN", &aip->sign);
	if (abs(aip->sign) != 1) {
	    tdlog ("DSIGN must be 1 or -1: %d", aip->sign);
	    exit(1);
	}
	readIVal ("DESIGN", &aip->esign);
	if (abs(aip->esign) != 1) {
	    tdlog ("DESIGN must be 1 or -1: %d", aip->esign);
	    exit(1);
	}
	readIVal ("DESTEP", &aip->estep);
	readDVal ("DTRENCWT", &aip->trencwt);
	if (aip->trencwt < 0 || aip->trencwt > 1) {
	    tdlog ("DTRENCWT must be 0..1: %d", aip->trencwt);
	    exit(1);
	}
	readDVal ("DLIMMARG", &tmp);
	aip->limmarg = tmp/(2*PI)*aip->step;
	readIVal ("DHOMEDIR", &aip->poshome);
	if (aip->poshome != 1 && aip->poshome != 0) {
	    tdlog ("DHOMEDIR must be 1 or 0: %d", aip->poshome);
	    exit(1);
	}
	if (readCFG ("DAXIS", value, sizeof(value)) < 0) {
	    tdlog ("No DAXIS in %s", tdcfg);
	    exit(1);
	}
	aip->axis = value[0];

	/* R axis */
	aip = raip;
	readIVal ("RHAVE", &aip->have);
	if (aip->have) {
	    readIVal ("RSTEP", &aip->step);
	    readDVal ("RMAXVEL", &tmp);
	    aip->maxvel = aip->step*tmp/(2*PI) + 0.5;
	    readDVal ("RMAXSACC", &tmp);
	    aip->maxsacc = aip->step*tmp/(2*PI) + 0.5;
	    readDVal ("RMAXTACC", &tmp);
	    aip->maxtacc = aip->step*tmp/(2*PI) + 0.5;
	    readIVal ("RSIGN", &aip->sign);
	    if (abs(aip->sign) != 1) {
		tdlog ("RSIGN must be 1 or -1: %d", aip->sign);
		exit(1);
	    }
	    aip->esign = aip->sign;	/* equivalent */
	    aip->estep = aip->step;	/* equivalent */
	    readDVal ("RTRENCWT", &aip->trencwt);
	    if (aip->trencwt < 0 || aip->trencwt > 1) {
		tdlog ("RTRENCWT must be 0..1: %d", aip->trencwt);
		exit(1);
	    }
	    readDVal ("DLIMMARG", &tmp);
	    readDVal ("RLIMMARG", &tmp);
	    aip->limmarg = tmp/(2*PI)*aip->step;
	    readIVal ("RHOMEDIR", &aip->poshome);
	    if (aip->poshome != 1 && aip->poshome != 0) {
		tdlog ("RHOMEDIR must be 1 or 0: %d", aip->poshome);
		exit(1);
	    }
	    if (readCFG ("RAXIS", value, sizeof(value)) < 0) {
		tdlog ("No RAXIS in %s", tdcfg);
		exit(1);
	    }
	    aip->axis = value[0];
	} else {
	    aip->rate = 0;
	    aip->pos = 0;
	    aip->targ = 0;
	    aip->pref = 0;
	    aip->rawpos = 0;
	}

	/* fetch the orientation and physical params */
	setConfigFile (hcfg, 1);

	readDVal ("HT", &telstatshmp->tax.HT);
	readDVal ("DT", &telstatshmp->tax.DT);
	readDVal ("XP", &telstatshmp->tax.XP);
	readDVal ("YC", &telstatshmp->tax.YC);
	readDVal ("NP", &telstatshmp->tax.NP);
	readDVal ("R0", &telstatshmp->tax.R0);

	aip = haip;
	readDVal ("HPOSLIM", &aip->poslim);
	readDVal ("HNEGLIM", &aip->neglim);
	telstatshmp->tax.HESTEP = aip->estep;
	telstatshmp->tax.HESIGN = aip->esign;

	aip = daip;
	readDVal ("DPOSLIM", &aip->poslim);
	readDVal ("DNEGLIM", &aip->neglim);
	telstatshmp->tax.DESTEP = aip->estep;
	telstatshmp->tax.DESIGN = aip->esign;

	aip = raip;
	if (aip->have) {
	    readDVal ("RPOSLIM", &aip->poslim);
	    readDVal ("RNEGLIM", &aip->neglim);
	}
}

/* do whatever to decide if things are sane.
 * if reset is set, then zero the history.
 * return 0 if ok, else -1.
 */
int
tel_sane_progress (int reset)
{
	/* TODO */
	return (0);
#if 0
	static int phrate, pdrate, prrate;	/* prior rates */
	static int phenc, pdenc, prstp;		/* prior positions */
	int hrate, drate, rrate;
	int henc, denc, rstp;
	int ret;

	/* reset is just 0 and done */
	if (reset) {
	     phrate = pdrate = prrate = 0;
	     phenc = pdenc = prstp = 0;
	     return (0);
	}

	/* handy values */
	hrate  = telstatshmp->hrate;
	drate  = telstatshmp->drate;
	rrate  = telstatshmp->rrate;
	henc  = telstatshmp->henc;
	denc  = telstatshmp->denc;
	rstp  = telstatshmp->rstp;
	ret =  0;

	/* make checks */
	switch (telstatshmp->telstatus) {
	case TS_STOPPED:	/* should at least be slowing down */
	    if (abs(hrate) > abs(phrate)) {
		sane_ratemsg (haip->axis, phrate, hrate);
		ret = -1;
	    }
	    if (abs(drate) > abs(pdrate)) {
		sane_ratemsg (daip->axis, pdrate, drate);
		ret = -1;
	    }
	    if (abs(rrate) > abs(prrate)) {
		sane_ratemsg (raip->axis, prrate, rrate);
		ret = -1;
	    }
	    break;

	default:		/* other states should see some progress. */
	    if (sane_position (haip, phrate, hrate, phenc, henc) < 0)
		ret = -1;
	    if (sane_position (daip, pdrate, drate, pdenc, denc) < 0)
		ret = -1;
	    if (sane_position (raip, pdrate, rrate, prstp, rstp) < 0)
		ret = -1;
	    break;
	}

	/* save current as "prior" for next time */
	phrate = hrate;
	pdrate = drate;
	prrate = rrate;
	phenc = henc;
	pdenc = denc;
	prstp = rstp;

	return (ret);
}

/* report a bad sanity rate status */
static void
sane_ratemsg (char axis, int priorrate, int currate)
{
	char buf[1024];
	int n;

	n = pc39_wr (buf, sizeof(buf), "a%c qa", axis);
	if (n < 4)
	    tdlog("Whoa! %c axis Stopped: prior rate=%d < rate=%d (no status)",
	    				axis, priorrate, currate);
	else
	    tdlog("Whoa! %c axis Stopped: prior rate=%d < rate=%d QA=%.*s",
					    axis, priorrate, currate, n, buf);
}

/* check and report a bad sanity position status.
 * return 0 if ok, else -1
 */
static int
sane_position (AxisInfo *aip, int prate, int rate, int ppos, int pos)
{
	int r;
	
	/* compute speed in units of encoder motion */
	r = (int)floor((rate * aip->sign * aip->esign)
					* ((double)aip->estep/aip->step) + 0.5);

	/* compare encoder rate with changes in encoder values */
	if ((r > 0 && pos <= ppos) || (r < 0 && pos >= ppos)) {
	    char buf[1024];
	    int n;

	    n = pc39_wr (buf, sizeof(buf), "a%c qa", aip->axis);
	    if (n < 4)
		tdlog ("Whoa! %c axis prate=%d rate=%d ppos=%d pos=%d",
					aip->axis, prate, rate, ppos, pos);
	    else
		tdlog ("Whoa! %c axis prate=%d rate=%d ppos=%d pos=%d QA=%.*s",
				    aip->axis, prate, rate, ppos, pos, n, buf);
	    return (-1);
	}

	return (0);
#endif 
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: telctrl.c,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $"};
