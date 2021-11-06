/* code to handle generic axis fucntions, such as homing and finding limits.
 */

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

#include "teled.h"

#define	MAXHV		1000	/* max raw speed to believe home, us/s */
#define	BACKTM		10	/* home approach takes about this long, secs */

static void recordLimit (MotorInfo *mip);
static void recordStep (MotorInfo *mip, int motdiff, int encdiff);

/* find home.
 * if have limits: go left to limit (so we can use a huge mr and hence use
 *   soft limits), go right to home, left a little, right slowly, record.
 * if no limits: just go right to home.
 * "left" and "right" are as per POSSIDE.
 * return 0 if in-progress, 1 done, else trouble logged to fid and return -1.
 */
int
axis_home (MotorInfo *mip, FifoId fid, int first)
{
	static double mjdto[TEL_NM];
	int i = mip - &telstatshmp->minfo[0];
	char axis = mip->axis;
	char buf[1024];

	/* sanity check */
	if (i < 0 || i >= TEL_NM) {
	    tdlog ("Bug!! bad mip sent to axis_home: %d\n", i);
	    die();
	}

	/* kindly oblige if not connected */
	if (!mip->have)
	    return (1);

	if (first) {
	    int posside = (mip->posside ? 1 : -1) * mip->sign;
	    int vl = mip->step * mip->maxvel/(2*PI);
	    int hv = MAXHV < vl ? MAXHV : vl;
	    char hl = mip->homelow ? 'l' : 'h';
	    char he = mip->enchome ? 'e' : 's';
	    char hm = posside > 0 ? 'm' : 'r';
	    int sac = mip->step * mip->slimacc/(2*PI);
	    int mac = mip->step * mip->maxacc/(2*PI);
	    int ma = -posside*(vl/MAXHV + BACKTM*hv);
			/* worst-case polling error + running start */

	    if (mip->havelim) {
		/* use limit, soft limits at that if desired */
		int mr = -2 * posside * mip->step;

		pc39_w ("a%c "		/* specify axis */
			"ic ln sl "	/* reset DONE, soft limits on */
			"ac%d vl%d "	/* set lively accel and vel */
			"h%c h%c "	/* set home polarity and whether enc */
			"mr%d go "	/* seek limit left of home */
			"ac%d h%c0 "	/* seek right to home and zero */
			"ma%d go "	/* get on left side again */
			"vl%d h%c0 "	/* seek right again, this time slowly */
			"ma0 go"	/* now really go there */
			"id",		/* report done */
			axis,
			sac, vl,
			hl, he,
			mr,
			mac, hm,	/* mac ok since home overshoot ok */
			ma,
			hv, hm);

	    } else {
		/* no limit -- just seek in posside direction */

		pc39_w ("a%c "		/* specify axis */
			"ic lf "	/* reset DONE, limits off */
			"ac%d vl%d "	/* set lively accel and vel */
			"h%c h%c "	/* set home polarity and whether enc */
			"ac%d h%c0 "	/* seek right to home and zero */
			"ma%d go "	/* get on left side again */
			"vl%d h%c0 "	/* seek right again, this time slowly */
			"ma0 go"	/* now really go there */
			"id",		/* report done */
			axis,
			sac, vl,
			hl, he,
			sac, hm,	/* use sac to minimize overshoot */
			ma,
			hv, hm);
	    }

	    /* public state */
	    mip->homing = 1;
	    mip->cvel = mip->maxvel;
	    mip->dpos = 0;

	    /* estimate a timeout -- only clue is initial limit estimates */
	    mjdto[i] = telstatshmp->now.n_mjd +
			(BACKTM+2*(mip->poslim-mip->neglim)/mip->maxvel)/SPD;
	}

	/* done when see DONE */
	pc39_wr (buf, sizeof(buf), "a%c qa", axis);
	if (nohw || strchr (buf, 'D')) {
	    pc39_w ("a%c ac%d ic", axis, MAXACCStp(mip));
	    mip->cvel = 0;
	    mip->homing = 0;
	    return (1);
	}

	/* check for timeout */
	if (telstatshmp->now.n_mjd > mjdto[i]) {
	    pc39_w ("a%c st", axis);
	    fifoWrite (fid, -1, "Axis %c timed out finding home", axis);
	    return (-1);
	}

	/* check for motion errors */
	if (axisMotionCheck (mip, buf) < 0) {
	    pc39_w ("a%c st", axis);
	    fifoWrite (fid, -2, "Axis %c homing motion error: %s ", axis, buf);
	    return (-1);
	}

	return (0);
}

/* set up the basic dynamical parameters */
void
setSpeedDefaults(MotorInfo *mip)
{
	if (mip->have)
	    pc39_w ("a%c ac%d vl%d", mip->axis, MAXACCStp(mip), MAXVELStp(mip));
}

/* check if the given axis is about to hit a limit.
 * return 0 if ok else write message in msgbuf[], stop and return -1.
 */
int
axisLimitCheck (MotorInfo *mip, char msgbuf[])
{
	double dt, nxtpos;

	/* check for this axis effectly not being used now */
	if (mip->cvel == 0)
	    return (0);

	/* check for predicted hit # cvel */
	dt = telstatshmp->dt/1000.0;
	nxtpos = mip->cpos + mip->cvel*dt;
	if (mip->cvel > 0 && nxtpos >= mip->poslim) {
	    pc39_w ("a%c ac%0.f st", mip->axis, mip->step*mip->slimacc/(2*PI));
	    sprintf (msgbuf, "Axis %c predicted to hit Pos limit", mip->axis);
	    return (-1);
	}
	if (mip->cvel < 0 && nxtpos <= mip->neglim) {
	    pc39_w ("a%c ac%0.f st", mip->axis, mip->step*mip->slimacc/(2*PI));
	    sprintf (msgbuf, "Axis %c predicted to hit Neg limit", mip->axis);
	    return (-1);
	}

	/* still ok */
	return (0);
}

/* check for sane motion for the given motor.
 * return 0 if ok, put reason in msgbuf and return -1
 */
int
axisMotionCheck (MotorInfo *mip, char msgbuf[])
{
#if 0
	/* TODO */
	static double last_cpos[TEL_NM];	/* last mip->cpos */
	static double last_changed[TEL_NM];	/* last mjd it changed */
	int i = mip - telstatshmp->minfo;
	double now = telstatshmp->now.n_mjd;
	double v;
	int stuck;

	/* can't do anything without an encoder to cross-check */
	if (!mip->have || !mip->haveenc)
	    return (0);

	/* sanity check mip */
	if (i < 0 || i >= TEL_NM) {
	    tdlog ("Bug!! bad mip sent to axisMotionCheck: %d\n", i);
	    die();
	}

	/* velocity */
	v = (mip->cpos - last_cpos[i])/(now - last_changed[i]);

	if (v > mip->cvel) {
	    /* too fast! */
	}
	if (!v && mip->cvel) {
	    /* stuck! */
	}

	/* record change */
	if (last_cpos[i] != mip->cpos) {
	    last_changed[i] = now;
	    last_cpos[i] = mip->cpos;
	}

	return (stuck ? -1 : 0);
#else
	return (0);
#endif
}

/* hunt for both axes' limits and record in home.cfg. if have an encoder, also
 * find and record new motor step scale.
 * return 0 if in-progress, 1 done, else trouble logged to fid and return -1.
 */
int
axis_limits (MotorInfo *mip, FifoId fid, int first) 
{
	static double mjdto[TEL_NM];	/* timeout */
	static char goal[TEL_NM];	/* limit we are seeking */
	static char lim[TEL_NM];	/* last limit we saw */
	static int motbeg[TEL_NM];	/* motor at beginning of sweep */
	static int encbeg[TEL_NM];	/* encoder at beginning of sweep */
	int i = mip - telstatshmp->minfo;
	char axis = mip->axis;
	char buf[1024], qa[32];
	char saw;

	if (!mip->havelim) {
	    fifoWrite(fid,0,"Axis %c: ok, but no limit switches are configured",
								    mip->axis);
	    return (-1);
	}

	/* sanity check */
	if (i < 0 || i >= TEL_NM) {
	    tdlog ("Bug!! bad mip sent to axis_limits: %d\n", i);
	    die();
	}

	/* kindly oblige if not connected */
	if (!mip->have)
	    return (1);

	if (first) {
	    int ac = mip->step * mip->slimacc/(2*PI);
	    int vl = mip->step * mip->maxvel/(2*PI);
	    int mr = 1.5*mip->step*(mip->poslim-mip->neglim)/(2*PI);
	    int dir;

	    /* pick a starting direction */
	    pc39_wr (qa, sizeof(qa), "a%c qa", axis);
	    if (strchr (qa, 'L')) {
		/* on a limit right now.. don't trust it being on the edge */
		int sawp = strchr (qa, 'P') != NULL;
		dir = sawp ? -mip->sign : mip->sign;
	    } else {
		/* nearest limit will be in opposite direction of home */
		dir = mip->cpos < 0 ? -1 : 1;
	    }

	    /* go */
	    lim[i] = '\0';
	    mip->cvel = dir*mip->maxvel;
	    goal[i] = dir*mip->sign < 0 ? 'M' : 'P';
	    pc39_w ("a%c "		/* specify axis */
		    "ln sl "		/* soft limits on */
		    "ac%d vl%d "	/* set accel and vel */
		    "mr%d go",		/* find first limit */
		    axis,
		    ac, vl,
		    mr*dir*mip->sign);

	    /* for the eavedroppers */
	    mip->limiting = 1;

	    /* estimate a timeout -- only clue is initial limit estimates */
	    mjdto[i] = telstatshmp->now.n_mjd +
				(2*(mip->poslim-mip->neglim)/mip->maxvel)/SPD;
	}

	/* check for timeout */
	if (telstatshmp->now.n_mjd > mjdto[i]) {
	    pc39_w ("a%c st", axis);
	    fifoWrite (fid, -3, "Axis %c timed out finding limits", axis);
	    return (-1);
	}

	/* check for motion errors */
	if (axisMotionCheck (mip, buf) < 0) {
	    pc39_w ("a%c st", axis);
	    fifoWrite(fid,-4, "Axis %c motion error finding limits: %s", axis,
	    								buf);
	    return (-1);
	}

	/* get fresh status */
	pc39_wr (qa, sizeof(qa), "a%c qa", axis);

	/* all the action is at the limits */
	if (!strchr (qa, 'L'))
	    return (0);
	    
	/* which limit */
	saw = strchr (qa, 'P') ? 'P' : 'M';

	/* wait until see the goal */
	if (goal[i] != saw)
	    return (0);

	if (!lim[i]) {
	    /* this is the first limit we have "run into" (sorry) */
	    int mr = 1.5*mip->step*(mip->poslim-mip->neglim)/(2*PI);

	    /* record */
	    lim[i] = saw;
	    recordLimit (mip);
	    if (mip->haveenc) {
		/* get motor and enc positions to find scale */
		pc39_wr (buf, sizeof(buf), "a%c rp", axis);
		motbeg[i] = atoi(buf);
		pc39_wr (buf, sizeof(buf), "a%c re", axis);
		encbeg[i] = atoi(buf);
	    }

	    /* turn around */
	    mip->cvel *= -1;
	    if (goal[i] == 'P') {
		goal[i] = 'M';
		pc39_w ("a%c ln sl mr%d go", axis, -mr);
	    } else {
		goal[i] = 'P';
		pc39_w ("a%c ln sl mr%d go", axis, mr);
	    }

	    fifoWrite (fid, 2, "Axis %c: %c limit found", axis, saw);
	    return (0);

	} else if(lim[i] != saw) {
	    /* ran into a different limit than before */
	    recordLimit (mip);
	    if (mip->haveenc) {
		/* get motor and enc again and compute/record scale */
		int motend, encend;
		pc39_wr (buf, sizeof(buf), "a%c rp", axis);
		motend = atoi(buf);
		pc39_wr (buf, sizeof(buf), "a%c re", axis);
		encend = atoi(buf);
		recordStep (mip, motend-motbeg[i], encend-encbeg[i]);
	    }

	    /* would like to back away but caller often issues stop and 
	     * rereads config file.
	     */

	    /* done */
	    mip->cvel = 0;
	    mip->limiting = 0;
	    fifoWrite (fid, 3, "Axis %c: %c limit found", axis, saw);
	    return (1);
	} else {
	    /* appears we keep seeing same limit on */
	    mip->cvel = 0;
	    mip->limiting = 0;
	    fifoWrite (fid, -5, "Axis %c: %c limit appears stuck on", axis,saw);
	    return (-1);
	}
}

/* we are currently at a limit -- record in config file */
static void
recordLimit (MotorInfo *mip)
{
	char name[64], valu[64];

	if (mip == &telstatshmp->minfo[TEL_HM])
	    name[0] = 'H';
	else if (mip == &telstatshmp->minfo[TEL_DM])
	    name[0] = 'D';
	else if (mip == &telstatshmp->minfo[TEL_RM])
	    name[0] = 'R';
	else if (mip == &telstatshmp->minfo[TEL_OM])
	    name[0] = 'O';
	else if (mip == &telstatshmp->minfo[TEL_IM])
	    name[0] = 'I';
	else {
	    /* who could it be?? */
	    tdlog ("Bogus mip passed to recordLimit: %ld", (long)mip);
	    return;
	}

	/* actually store limmarg inside */
	if (mip->cvel > 0) {
	    strcpy (name+1, "POSLIM");
	    sprintf (valu, "%.6f", mip->cpos - mip->limmarg);
	} else {
	    strcpy (name+1, "NEGLIM");
	    sprintf (valu, "%.6f", mip->cpos + mip->limmarg);
	}

	if (writeCfgFile (hcfn, name, valu, NULL) < 0)
	    tdlog ("%s: %s in recordLimit", hcfn, name);
}

/* compute and record a new motor step and sign */
static void
recordStep (MotorInfo *mip, int motdiff, int encdiff)
{
	char name[64], valu[64];

	if (!mip->haveenc)
	    return;

	if (mip == &telstatshmp->minfo[TEL_HM])
	    name[0] = 'H';
	else if (mip == &telstatshmp->minfo[TEL_DM])
	    name[0] = 'D';
	else {
	    tdlog ("Bogus mip passed to recordStep: %ld", (long)mip);
	    return;
	}

	strcpy (name+1, "STEP");
	sprintf (valu, "%.1f", fabs((double)mip->estep*motdiff/encdiff));
	if (writeCfgFile (hcfn, name, valu, NULL) < 0)
	    tdlog ("%s: %s in recordStep", hcfn, name);

	strcpy (name+1, "SIGN");
	sprintf (valu, "%d", (double)motdiff*encdiff > 0 ?
						    mip->esign : -mip->esign);
	if (writeCfgFile (hcfn, name, valu, NULL) < 0)
	    tdlog ("%s: %s in recordStep", hcfn, name);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: axes.c,v $ $Date: 2003/04/15 20:47:58 $ $Revision: 1.1.1.1 $ $Name:  $"};
