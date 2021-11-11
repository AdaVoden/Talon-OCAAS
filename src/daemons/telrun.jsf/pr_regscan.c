/* program to execute a "regular" scan.
 * this is one with CCDCALIB either NONE or CATALOG.
 * waiting for download and postproc is broken off into a separate program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "telenv.h"
#include "telstatshm.h"
#include "scan.h"

#include "telrun.h"

static int pr_startPP (int first);
static int postProcess(void);
static void logStart(time_t n);

static char pplog[] = "archive/logs/postprocess.log";	/* pp log file */

static time_t tmpTime;		/* used to save times from one step to next */
static Scan bkg_scan;		/* used for background program */

typedef void *StepFuncP;
typedef StepFuncP (*StepFunc)(time_t now);

static StepFuncP ps_wait4Start(time_t n);
static StepFuncP ps_wait4Setup(time_t n);
static StepFuncP ps_wait4Exp (time_t n);

/* program to process a regular scan. called periodically by main_loop().
 * first is only set when just starting this scan.
 * return 0 if making progress, -1 on error or finished.
 * once expose completes, a separate program waits for download and starts
 *   postprocressing.
 * N.B. we are responsible for all our own logging and cleanup on errors.
 * N.B. due to circumstances beyond our control we may never get called again.
 */
int
pr_regscan (int first)
{
	static StepFuncP step;

	/* start over if first call */
	if (first) {
	    step = ps_wait4Start;	/* init sequencer */
	}

	/* run this step, next is its return */
	step = step ? (*(void *(*)())step)(time(NULL)) : NULL;

	if (!step) {
	    cscan->running = 0;
	    cscan->starttm = 0;
	    return (-1);
	}

	/* still working */
	return (0);
}

/* get things moving towards taking the scan defined in cscan */
void
pr_regSetup()
{
	Scan *sp = cscan;
	char buf[1024];
	int n;

	/* start telescope */
	n = sprintf (buf, "dRA:%.6f dDec:%.6f #", sp->rao, sp->deco);
	db_write_line (&sp->obj, buf+n);
	fifoWrite (Tel_Id, buf);

	/* start filter */
	if (IMOT->have) {
	    buf[0] = sp->filter;
	    buf[1] = '\0';
	    fifoWrite (Filter_Id, buf);
	}

	/* insure focus is participating */
	if (OMOT->have && !telstatshmp->autofocus)
	    fifoWrite (Focus_Id, "Auto");

	/* insure dome is open and going */
	if (telstatshmp->shutterstate != SH_ABSENT && 
				telstatshmp->shutterstate != SH_OPEN &&
				telstatshmp->shutterstate != SH_OPENING)
	    fifoWrite (Dome_Id, "Open");
	if (telstatshmp->domestate != DS_ABSENT && !telstatshmp->autodome)
	    fifoWrite (Dome_Id, "Auto");
}

/* decide whether to start cscan */
static StepFuncP
ps_wait4Start(time_t n)
{
	Scan *sp = cscan;
	char buf[64];

	/* check for nominally near starttm */
	if (n > sp->starttm + sp->startdt) {
	    n -= sp->starttm + sp->startdt;
	    tlog (sp, "Too late by %d secs", (int) n);
	    markScan (scanfile, sp, 'F');
	    return (NULL);
	}
	if (n < sp->starttm - SETUP_TO)
	    return (ps_wait4Start);		/* waiting */
	
	if (n > sp->starttm) // if no time to setup
	{
		n -= sp->starttm;
	    tlog (sp, "Starting too late by %d secs", (int) n);
	    markScan (scanfile, sp, 'F');
		return(NULL);	  // abort
	}		

	/* go! */
	strcpy (buf, timestamp(sp->starttm));	/* save from tlog */
	tlog (sp, "Checking scan setup.. scheduled at %s", buf);
	pr_regSetup();

	/* set a trigger for the start time */
	setTrigger (sp->starttm);

	/* now wait for setup and time to start */
	tmpTime = n + SETUP_TO;
	return (ps_wait4Setup);
}

/* wait for all required systems to come ready then start camera */
static StepFuncP
ps_wait4Setup(time_t n)
{
	int camok = telstatshmp->camstate == CAM_IDLE;
	int telok = telstatshmp->telstate == TS_TRACKING;
	int filok = FILTER_READY;
	int focok = FOCUS_READY;
	int domok = DOME_READY;
	int allok = camok && telok && filok && focok && domok;
	Scan *sp = cscan;
	char fullpath[1024];

	/* trouble if not finished within SETUP */
	if (!allok && n > tmpTime) {
	    tlog (sp, "Setup timed out for%s%s%s%s%s",
						    focok ? "" : " focus",
						    camok ? "" : " camera",
						    telok ? "" : " telescope",
						    filok ? "" : " filter",
						    domok ? "" : " dome");
	    all_stop(1);
	    return (NULL);
	}

	/* don't wait past allowable dt in any case */
	if (n > sp->starttm + sp->startdt) {
	    n -= sp->starttm + sp->startdt;
	    tlog (sp, "Setup too late by %d secs", (int) n);
	    all_stop(1);
	    return (NULL);
	}

	/* start no sooner than original start time */
	if (!allok || n < sp->starttm)
	    return (ps_wait4Setup);

	/* Ok! */

	/* publish */
	logStart(n);
	sp->starttm = n;		/* real start time */
	sp->running = 1;		/* we are under way */

	/* start camera */
	sprintf (fullpath, "%s/%s", sp->imagedn, sp->imagefn);
	fifoWrite (Cam_Id,
		    "Expose %d+%dx%dx%d %dx%d %g %d %d %s\n%s\n%s\n%s\n%s\n",
			sp->sx, sp->sy, sp->sw, sp->sh, sp->binx, sp->biny,
			    sp->dur, sp->shutter, sp->priority, fullpath,
			sp->obj.o_name,
			sp->comment,
			sp->title,
			sp->observer);

	/* save estimate of finish time */
	tmpTime = n + (int)floor(sp->dur + 0.5);

	/* ok */
	return (ps_wait4Exp);
}

/* wait for exposure to finish -- ok if idle or digitizing */
static StepFuncP
ps_wait4Exp (time_t n)
{
	int telok = telstatshmp->telstate == TS_TRACKING;
	int filok = FILTER_READY;
	int focok = FOCUS_READY;
	int domok = DOME_READY;
	Scan *sp = cscan;

	/* double-check support systems */
	if (!telok || !filok || !domok || !focok) {
	    tlog (sp, "Error during exposure from%s%s%s%s",
						    focok ? "" : " focus",
						    telok ? "" : " telescope",
						    filok ? "" : " filter",
						    domok ? "" : " dome");
	    all_stop(1);
	    return (NULL);
	}

	/* wait for completion..
	 * we could be a little early -- or so fast it is still IDLE
	 */
	if (n <= tmpTime || telstatshmp->camstate == CAM_EXPO)
	    return (ps_wait4Exp);	/* waiting... */

	/* exposure is finished: copy scan info to bkg_scan and start a
	 * new program to wait for download and start postprocessing.
	 * marking 'D' is really just to prevent this scan from being repeated.
	 * can still be marked 'F' if starting postprocess fails.
	 */
	fifoWrite (Tel_Id, "Stop");	/* stop tracking */
	tlog (sp, "Exposure complete.. starting download of %s", sp->imagefn);
	markScan (scanfile, sp, 'D');
	bkg_scan = *sp;
	tmpTime = n + CAMDIG_MAX;	/* estimate time at download complete */
	if (addProgram (pr_startPP, 1) < 0) {
	    tlog (&bkg_scan, "Error starting postprocess");
	    markScan (scanfile, sp, 'F');
	}
	
	/* all up to post processing now */
	return (NULL);
}

/* program to wait for download to complete and start postprocessing.
 * first is only set when just starting this action.
 * return 0 if making progress, -1 on error or finished.
 * N.B. we are responsible for all our own logging and cleanup on errors.
 * N.B. due to circumstances beyond our control we may never get called again.
 * N.B. scan being worked on here is bkg_scan, not cscan!
 */
static int
pr_startPP (int first)
{
	Scan *sp = &bkg_scan;

	switch (telstatshmp->camstate) {
	case CAM_EXPO:	/* on to next already -- well, ours is done then */
	case CAM_IDLE:
	    if (postProcess() == 0) {
		markScan (scanfile, sp, 'D');
		return (-1);	/* remove from program q */
	    }
	    break;

	case CAM_READ:
	    if (time(NULL) < tmpTime)
		return (0);	/* stay in program q */
	    tlog (sp, "Camera READING too long");
	    break;
	}

	/* trouble if get here -- already logged why */
	markScan (scanfile, sp, 'F');
	return (-1);	/* remove from program q */
}


/* helper funcs */

/* start the post-processing work for bkg_scan.
 * return 0 if gets off to a good start, else -1.
 */
static int
postProcess ()
{
	Scan *sp = &bkg_scan;
	char cmd[2048];
	char pp[1024];
	int s;

	telfixpath (pp, pplog);
	tlog (sp, "Starting postprocess %s cor=%d scale=%d", sp->imagefn, 
			    sp->ccdcalib.data == CD_COOKED, sp->compress);
	sprintf (cmd, "nice postprocess %s/%s %d %d >> %s 2>&1 &",
						sp->imagedn, sp->imagefn,
			    sp->ccdcalib.data == CD_COOKED, sp->compress, pp);
	if ((s = system(cmd)) != 0) {
	    tlog (sp, "Postprocess script failed: %d", s);
	    return (-1);
	}
	return (0);
}

/* log all the starting telescope particulars */
static void
logStart(time_t n)
{
	MotorInfo *mip;
	Scan *sp = cscan;
	char rabuf[32], decbuf[32];
	char altbuf[32], azbuf[32];
	char buf[256], *bp;
	int late;
	int bl;

	/* lead-in message */
	late = n - sp->starttm;
	if (late)
	    tlog (sp, "Starting exposure.. late by %d sec%s", late,
							late == 1 ? "" : "s");
	else
	    tlog (sp, "Starting exposure on time");

	/* log object definition, sans name since that is from sp already  */
	db_write_line (&sp->obj, buf);
	bp = strchr (buf, ',');
	if (bp)
	    tlog (sp, "%s", bp+1);

	/* log telescope and camera info for sure */
	fs_sexa (rabuf, radhr(telstatshmp->CJ2kRA), 3, 36000);
	fs_sexa (decbuf, raddeg(telstatshmp->CJ2kDec), 3, 36000);
	fs_sexa (altbuf, raddeg(telstatshmp->Calt), 3, 36000);
	fs_sexa (azbuf, raddeg(telstatshmp->Caz), 3, 36000);

	tlog (sp, "  Telescope RA/Dec: %s %s J2000", rabuf, decbuf);
	if (sp->rao || sp->deco) {
	    fs_sexa (rabuf, radhr(sp->rao), 3, 36000);
	    fs_sexa (decbuf, raddeg(sp->deco), 3, 36000);
	    tlog (sp, "     Offset RA/Dec: %s %s", rabuf, decbuf);
	}
	tlog (sp, "  Telescope Alt/Az: %s %s", altbuf, azbuf);

	tlog (sp, "  Camera: %c +%d+%dx%dx%d %dx%d %gs %s Pr%d %s",
			sp->filter, sp->sx, sp->sy, sp->sw, sp->sh, sp->binx,
		    sp->biny, sp->dur, ccdSO2Str(sp->shutter), sp->priority,
		    sp->imagefn);

	/* remaining are optional parts */
	bl = 0;

	mip = IMOT;
	if (mip->have)
	    bl += sprintf (buf+bl, " Filter: %c", telstatshmp->filter);

	mip = OMOT;
	if (mip->have)
	    bl += sprintf (buf+bl, " Focus: %g",
				mip->step * mip->cpos / mip->focscale / (2*PI));

	if (telstatshmp->domestate != DS_ABSENT) {
	    fs_sexa (rabuf, raddeg(telstatshmp->domeaz), 4, 3600);
	    bl += sprintf (buf+bl, " Dome: %s", rabuf);
	}

	switch (telstatshmp->shutterstate) {
	case SH_ABSENT:
	    break;
	case SH_OPEN:
	    bl += sprintf (buf+bl, " Roof: open");
	    break;
	default:
	    bl += sprintf (buf+bl, " Roof: %d", telstatshmp->shutterstate);
	    break;
	}

	if (bl > 0)
	    tlog (sp, " %s", buf);

}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pr_regscan.c,v $ $Date: 2003/04/15 20:48:01 $ $Revision: 1.1.1.1 $ $Name:  $"};
