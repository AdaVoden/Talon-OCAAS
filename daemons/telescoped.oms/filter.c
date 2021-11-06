/* handle the Filter channel.
 * if IHAVETORUS handle the Torus brand, else a generic wheel or tray on a
 * stepper.
 */

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

/* the current activity, if any */
static void (*active_func) (int first, ...);

/* one of these... */
static void filter_poll(void);
static void filter_reset(int first);
static void filter_home(int first, ...);
static void filter_limits(int first, ...);
static void filter_stop(int first, ...);
static void filter_set(int first, ...);
static void filter_jog(int first, ...);

/* helped along by these... */
static void initCfg(void);
static void stopFilter(int fast);
static void readFilter (void);
static void showFilter (void);
static int readFilInfo(void);

/* filled with filter/focus/temp info */
static FilterInfo *filinfo;
static int nfilinfo;

static int I1STEP;
static int IOFFSET;
static int IHAVETORUS;
static int deffilt;

/* Torus stuff */
#define	TORTO		(20/SPD)	/* overall timeout, days */
#define	TORHT		(2/SPD)		/* wait before testing for home, days */
#define	TORST		(1.5/SPD)	/* final settling time, days */
#define	TP2POS(p)	(2*PI*(p)/12)	/* index to angle */
static int tpos;			/* current position, index */

/* called when we receive a message from the Filter fifo.
 * if !msg just update things.
 */
/* ARGSUSED */
void
filter_msg (msg)
char *msg;
{
	char jog[10];

	/* do reset before checking for `have' to allow for new config file */
	if (msg && strncasecmp (msg, "reset", 5) == 0) {
	    filter_reset(1);
	    return;
	}

	if (!IMOT->have) {
	    if (msg)
		fifoWrite (Filter_Id, 0, "Ok, but filter not really installed");
	    return;
	}

	if (!msg)
	    filter_poll();
	else if (strncasecmp (msg, "home", 4) == 0)
	    filter_home(1);
	else if (strncasecmp (msg, "stop", 4) == 0)
	    filter_stop(1);
	else if (strncasecmp (msg, "limits", 6) == 0)
	    filter_limits(1);
	else if (sscanf (msg, "j%1[0+-]", jog) == 1)
	    filter_jog (1, jog[0]);
	else
	    filter_set (1, msg);
}

/* search the filter info for the given name.
 * if name == '\0' return the default filter.
 * return matching FilterInfo, or null if not in list, or die() if no list.
 */
FilterInfo *
findFilter (char name)
{
	FilterInfo *fip, *lfip;

	if (!filinfo && readFilInfo() < 0)
	    die();

	if (name == '\0')
	    return (&filinfo[deffilt]);

	if (islower(name))
	    name = toupper(name);
	lfip = &filinfo[nfilinfo];
	for (fip = filinfo; fip < lfip; fip++) {
	    char n = fip->name[0];
	    if (islower(n))
		n = toupper(n);
	    if (name == n)
		return (fip);
	}

	return (NULL);
}

/* no new messages.
 * goose the current objective, if any.
 */
static void
filter_poll()
{
	if (active_func)
	    (*active_func)(0);
}

/* stop and reread config files */
static void
filter_reset(int first)
{
	initCfg();

	if (IHAVETORUS) {
	    MotorInfo *mip = IMOT;

	    tpos = 0;
	    pc39_w ("a%c hh vl12 lp0 mr%d go", mip->axis, tpos+1);
	    telstatshmp->filter = filinfo[tpos].name[0];
	    mip->cvel = 0;
	    mip->cpos = TP2POS(tpos);
	    mip->dpos = TP2POS(tpos);
	    mip->raw = tpos+1;
	} else if (IMOT->have) {
	    stopFilter(0);
	    readFilter();
	    showFilter();
	}

	fifoWrite (Filter_Id, 0, "Reset complete");
}

/* seek the home position */
static void
filter_home(int first, ...)
{
	MotorInfo *mip = IMOT;

	if (IHAVETORUS) {
	    /* pretend we went home, then go to default filter */
	    if (first) {
		mip->homing = 1;
		fifoWrite (Filter_Id, 1, "Homing complete. Now going to %s.",
							filinfo[deffilt].name);
		toTTS ("The filter wheel has found home and is now going to the %s position.",
							filinfo[deffilt].name);
		filter_set (1, filinfo[deffilt].name);
		mip->homing = 0;
	    }

	    return;
	}

	if (first) {
	    if (axis_home (mip, Filter_Id, 1) < 0) {
		stopFilter(1);
		return;
	    }

	    /* new state */
	    active_func = filter_home;
	}

	switch (axis_home (mip, Filter_Id, 0)) {
	case -1:
	    stopFilter(1);
	    active_func = NULL;
	    return;
	case  0:
	    break;
	case  1: 
	    active_func = NULL;
	    fifoWrite (Filter_Id, 1, "Homing complete. Now going to %s.",
							filinfo[deffilt].name);
	    toTTS ("The filter wheel has found home and is now going to the %s position.",
							filinfo[deffilt].name);
	    filter_set (1, filinfo[deffilt].name);
	    break;
	}
}

static void
filter_limits(int first, ...)
{
	MotorInfo *mip = IMOT;

	if (IHAVETORUS) {
	    fifoWrite (Filter_Id,0,"Ok, but Torus filter does not have limits");
	    return;
	}

	/* maintain cpos and raw */
	readFilter();

	if (first) {
	    if (axis_limits (mip, Filter_Id, 1) < 0) {
		stopFilter(1);
		active_func = NULL;
		return;
	    }

	    /* new state */
	    active_func = filter_limits;
	}

	switch (axis_limits (mip, Filter_Id, 0)) {
	case -1:
	    stopFilter(1);
	    active_func = NULL;
	    return;
	case  0:
	    break;
	case  1: 
	    stopFilter(0);
	    active_func = NULL;
	    initCfg();		/* read new limits */
	    fifoWrite (Filter_Id, 0, "Limits found");
	    toTTS ("The filter wheel has found both limit positions.");
	    break;
	}
}

static void
filter_stop(int first, ...)
{
	MotorInfo *mip = IMOT;
	char buf[32];

	if (IHAVETORUS) {
	    fifoWrite (Filter_Id, 0,"Ok, but can not really stop Torus filter");
	    return;
	}

	/* stay current */
	readFilter();

	if (first) {
	    /* issue stop */
	    stopFilter(0);
	    active_func = filter_stop;
	}

	/* wait for really stopped */
	pc39_wr (buf, sizeof(buf), "a%c rv", mip->axis);
	if (atoi(buf))
	    return;

	/* if get here, it has really stopped */
	active_func = NULL;
	readFilter();
	showFilter();
	fifoWrite (Filter_Id, 0, "Stop complete");
}

/* handle setting a given filter position */
static void
filter_set(int first, ...)
{
	static int rawgoal;
	MotorInfo *mip = IMOT;
	char axis = mip->axis;
	char qa[32];
	
	if (IHAVETORUS) {
	    static double tomjd, htmjd, stmjd;
	    Now *np = &telstatshmp->now;

	    if (first) {
		FilterInfo *fip;
		va_list ap;
		char new;

		/* fetch new filter name */
		va_start (ap, first);
		new = *(va_arg (ap, char *));
		va_end (ap);

		/* always need filter info */
		if (!filinfo && readFilInfo() < 0)
		    die();

		/* find name in list */
		fip = findFilter (new);
		if (!fip) {
		    fifoWrite (Filter_Id, -7, "No filter is named %c", new);
		    return;
		}

		/* save and issue that many steps */
		tpos = fip - filinfo;
		pc39_w ("a%c hh vl12 lp0 mr%d go", axis, tpos + 1);
		mip->cvel = .52359;	/* 12 seconds/rev */
		mip->cpos = 0;
		mip->dpos = TP2POS(tpos);
		mip->raw = 1;
		telstatshmp->filter = '>';
		active_func = filter_set;
		tomjd = mjd + TORTO;
		htmjd = mjd + TORHT;
		stmjd = 0;
		toTTS ("The filter wheel is rotating to the %s position.",
								    fip->name);
	    }

	    /* start checking for home after htmjd */
	    if (mjd > htmjd) {
		/* finished when home comes up and stays up TORST */
		pc39_wr (qa, sizeof(qa), "a%c qa", axis);
		if (strchr (qa, 'H')) {
		    if (stmjd == 0)
			stmjd = mjd + TORST;
		    else if (mjd > stmjd) {
			active_func = NULL;
			mip->cvel = 0;
			mip->cpos = TP2POS(tpos);
			mip->dpos = TP2POS(tpos);
			mip->raw = tpos+1;
			telstatshmp->filter = filinfo[tpos].name[0];
			fifoWrite (Filter_Id, 0, "Filter in place");
			toTTS ("The filter wheel is in position.");
            telstatshmp->autofocus = 1; // autofocus on when new filter set
		    }
		}
	    }

	    /* check for timeout */
	    if (mjd > tomjd) {
		active_func = NULL;
		mip->cvel = 0;
		mip->cpos = 0;
		mip->dpos = 0;
		mip->raw = 0;
		telstatshmp->filter = '?';
		fifoWrite (Filter_Id, -8, "Timeout");
		toTTS ("The filter wheel has timed out.");
	    }

	    return;
	}

	/* maintain current info */
	readFilter();

	if (first) {
	    va_list ap;
	    FilterInfo *fip;
	    double goal;
	    char new;

	    /* fetch new filter name */
	    va_start (ap, first);
	    new = *(va_arg (ap, char *));
	    va_end (ap);

	    /* always need filter info */
	    if (!filinfo && readFilInfo() < 0)
		die();

	    /* find name in list */
	    fip = findFilter (new);
	    if (!fip) {
		fifoWrite (Filter_Id, -7, "No filter is named %c", new);
		return;
	    }

	    /* compute raw goal */
	    rawgoal = IOFFSET + (fip - filinfo)*I1STEP;

	    /* compute canonical goal */
	    goal = (2*PI)*mip->sign*rawgoal/mip->step;
	    if (mip->havelim) {
		if (goal > mip->poslim) {
		    fifoWrite (Filter_Id, -1, "Hits positive limit");
		    return;
		}
		if (goal < mip->neglim) {
		    fifoWrite (Filter_Id, -2, "Hits negative limit");
		    return;
		}
	    }

	    /* ok, go for the gold, er, goal */
	    setSpeedDefaults (mip);
	    pc39_w ("a%c l%c ma%d go", axis, mip->havelim?'n':'f', rawgoal);
	    mip->cvel = mip->sign*mip->maxvel;
	    mip->dpos = goal;
	    active_func = filter_set;
	    toTTS ("The filter wheel is rotating to the %s position.",
								    fip->name);
	}

	/* already checked within soft limits when set up move */

	/* done when step is correct */
	if (abs(mip->raw - rawgoal) <= 1) {
	    active_func = NULL;
	    stopFilter(1);
	    fifoWrite (Filter_Id, 0, "Filter in place");
	    toTTS ("The filter wheel is in position.");
	}
}

/* jog the given direction */
static void
filter_jog(int first, ...)
{
	MotorInfo *mip = IMOT;

	if (IHAVETORUS) {
	    va_list ap;
	    char jogcode;

	    /* fetch jog direction code */
	    va_start (ap, first);
	    jogcode = va_arg(ap, int);
	    va_end (ap);

	    /* ignore if not still */
	    if (mip->cvel != 0) {
		fifoWrite (Filter_Id, 1, "Prior jog still in progress...");
		return;
	    }

	    switch (jogcode) {
	    case '+':
		/* advance 1 from tpos */
		filter_set (1, filinfo[(tpos+1)%nfilinfo].name[0]);
		break;
	    case '-':
		/* retreat 1 from tpos */
		filter_set (1, filinfo[(tpos+nfilinfo-1)%nfilinfo].name[0]);
		break;
	    case '0':
		break;
	    default:
		fifoWrite (Filter_Id, -3, "Unknown jog code: %c", jogcode);
		return;
	    }

	    return;
	}

	/* maintain current info */
	readFilter();
	showFilter();

	if (first) {
	    va_list ap;
	    char jogcode;

	    /* fetch jog direction code */
	    va_start (ap, first);
	    jogcode = va_arg(ap, int);
	    va_end (ap);

	    /* ignore if not still */
	    if (mip->cvel != 0)
		return;

	    switch (jogcode) {
	    case '+':
		/* advance to next whole pos, with wrap */
		break;
	    case '-':
		/* advance to prior whole pos, with wrap */
		break;
	    case '0':
		filter_stop(1);
		return;
	    default:
		fifoWrite (Filter_Id, -3, "Unknown jog code: %c", jogcode);
		return;
	    }
	}

	fifoWrite (Filter_Id, -4, "Jog not yet implemented");
}


static void
initCfg()
{
#define NICFG   (sizeof(icfg)/sizeof(icfg[0]))
#define NHCFG   (sizeof(hcfg)/sizeof(hcfg[0]))

	static int IHAVE, IHASLIM;
	static int ISTEP, ISIGN, IPOSSIDE, IHOMELOW;
	static double ILIMMARG, IMAXVEL, IMAXACC, ISLIMACC;
	static char IAXIS;

	static CfgEntry icfg[] = {
	    {"IAXIS",		CFG_STR, &IAXIS, 1}, 
	    {"IHAVETORUS",	CFG_INT, &IHAVETORUS},
	    {"IHAVE",		CFG_INT, &IHAVE},
	    {"IHASLIM",		CFG_INT, &IHASLIM},
	    {"IPOSSIDE",	CFG_INT, &IPOSSIDE},
	    {"IHOMELOW",	CFG_INT, &IHOMELOW},
	    {"ISTEP",		CFG_INT, &ISTEP},
	    {"ISIGN",		CFG_INT, &ISIGN},
	    {"I1STEP",		CFG_INT, &I1STEP},
	    {"IOFFSET",		CFG_INT, &IOFFSET},
	    {"ILIMMARG",	CFG_DBL, &ILIMMARG},
	    {"IMAXVEL",		CFG_DBL, &IMAXVEL},
	    {"IMAXACC",		CFG_DBL, &IMAXACC},
	    {"ISLIMACC",	CFG_DBL, &ISLIMACC},
	};

	static double IPOSLIM, INEGLIM;

	static CfgEntry hcfg[] = {
	    {"IPOSLIM",		CFG_DBL, &IPOSLIM}, 
	    {"INEGLIM",		CFG_DBL, &INEGLIM}, 
	};

	MotorInfo *mip = IMOT;
	int n;

	n = readCfgFile (1, icfn, icfg, NICFG);
	if (n != NICFG) {
	    cfgFileError (icfn, n, (CfgPrFp)tdlog, icfg, NICFG);
	    die();
	}
	n = readCfgFile (1, hcfn, hcfg, NHCFG);
	if (n != NHCFG) {
	    cfgFileError (hcfn, n, (CfgPrFp)tdlog, hcfg, NHCFG);
	    die();
	}

	memset ((void *)mip, 0, sizeof(*mip));
	mip->axis = IAXIS;

	if (IHAVETORUS) {
	    mip->have = 1;
	    mip->haveenc = 0;
	    mip->enchome = 0;
	    mip->havelim = 0;
	    mip->posside = 1;
	    mip->homelow = 1;
	    mip->step = 2*PI;	/* so units are effectively steps */
	    mip->sign = 1;

	    mip->limmarg = 0;
	    mip->maxvel = 1;	/* must run at 1 HZ */
	    mip->maxacc = 1000;	/* start without delay */
	    mip->slimacc = 1000;/* start without delay */
	    mip->poslim = 11;	/* 12 positions at 0..11 */
	    mip->neglim = 0;

	    I1STEP = 1;		/* 1 step per filter */
	    IOFFSET = 1;	/* steps go 1..12 but filinfo[] indexed 0..11 */
	} else {
	    mip->have = IHAVE;
	    mip->haveenc = 0;
	    mip->enchome = 0;
	    mip->havelim = IHASLIM;
	    mip->posside = IPOSSIDE ? 1 : 0;
	    mip->homelow = IHOMELOW ? 1 : 0;
	    mip->step = ISTEP;

	    if (abs(ISIGN) != 1) {
		fprintf (stderr, "ISIGN must be +-1\n");
		die();
	    }
	    mip->sign = ISIGN;

	    mip->limmarg = ILIMMARG;
	    mip->maxvel = fabs(IMAXVEL);
	    mip->maxacc = IMAXACC;
	    mip->slimacc = ISLIMACC;
	    mip->poslim = IPOSLIM;
	    mip->neglim = INEGLIM;
	}

	/* (re)read fresh filter info */
	if (readFilInfo() < 0)
	    die();

#undef NICFG
#undef NHCFG
}

static void
stopFilter(int fast)
{
	MotorInfo *mip = IMOT;

	int ac = mip->step * (fast ? mip->slimacc : mip->maxacc)/(2*PI);

	pc39_w ("a%c ac%d st", mip->axis, ac);
	mip->cvel = 0;
	mip->homing = 0;
	mip->limiting = 0;
	readFilter();
	showFilter();
}

/* read the raw value */
static void
readFilter ()
{
	MotorInfo *mip = IMOT;
	char buf[32];

	if (!mip->have)
	    return;

	if (nohw) {
	    mip->cpos = mip->dpos;
	    mip->raw = (int)floor(mip->sign*mip->step*mip->cpos/(2*PI) + .5);
	} else {
	    pc39_wr (buf, sizeof(buf), "a%c rp", mip->axis);
	    mip->raw = atoi(buf);
	    mip->cpos = (2*PI) * mip->sign * mip->raw / mip->step;
	}
}

/* fill telescope->filter */
static void
showFilter()
{
	MotorInfo *mip = IMOT;

	/* always need filter info */
	if (!filinfo && readFilInfo() < 0)
	    die();

	if (mip->cvel > 0) {
	    telstatshmp->filter = '>';
	} else if (mip->cvel < 0) {
	    telstatshmp->filter = '<';
	} else {
	    int fs = (mip->raw - IOFFSET)/I1STEP;
        int fm;
#if JSF_HALFM
        fm = 0;
#else
        fm = (mip->raw - IOFFSET)%I1STEP;
#endif
	    if (fs < 0 || fs >= nfilinfo || fm != 0)
		telstatshmp->filter = '?';
	    else
		telstatshmp->filter = filinfo[fs].name[0];
	}
}

/* read icfn and fill in filinfo.
 * return 0 if ok, else write to Filter_Id and -1.
 */
static int
readFilInfo()
{
	char errmsg[1024];

	nfilinfo = readFilterCfg (1, icfn, &filinfo, &deffilt, errmsg);
	if (nfilinfo <= 0) {
	    if (nfilinfo < 0)
		fifoWrite (Filter_Id, -5, "%s", errmsg);
	    else
		fifoWrite (Filter_Id, -6, "%s: no entries", basenm(icfn));
	    return (-1);
	}
	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: filter.c,v $ $Date: 2003/04/15 20:48:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
