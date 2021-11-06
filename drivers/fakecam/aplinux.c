/* glue between the linux driver and the (almost) generic Apogee API functions.
 * (C) Copyright 1997 Elwood Charles Downey. All right reserved.
 * 6/25/97
 */

#include "apdata.h"
#include "apglob.h"
#include "apccd.h"
#include "aplow.h"

#include "aplinux.h"

#define	FAKE_AMB	20
static int fake_temp = FAKE_AMB, fake_temptarg = FAKE_AMB, fake_tempon;

/* probe for a camera at the given address.
 * return 1 if the device appears to be present, else return 0
 */
int
apProbe(int base)
{
	return (1);
}

/* set the camdata[] entry for the given camera from the module params.
 * since we can't actually read the camera type from the hardware, these
 * could be entirely bogus and we'd never know.
 * we set up the ROI for full chip (minus a 2-bit border), 1:1 binning.
 * return 0 if ok, else -1
 */
int
apInit (int minor, int params[])
{
	int hccd = MIN2HCCD(minor);
	CAMDATA *cdp = &camdata[minor];
	SHORT rows = (SHORT) params[APP_ROWS];
	SHORT cols = (SHORT) params[APP_COLS];
	SHORT topb = (SHORT) params[APP_TOPB];
	SHORT botb = (SHORT) params[APP_BOTB];
	SHORT leftb = (SHORT) params[APP_LEFTB];
	SHORT rightb = (SHORT) params[APP_RIGHTB];

	if (ap_trace) {
	    printk ("Ap @ 0x%x config params:\n", params[APP_IOBASE]);
	    printk ("  %5d %s\n", params[APP_ROWS],      "APP_ROWS");
	    printk ("  %5d %s\n", params[APP_COLS],      "APP_COLS");
	    printk ("  %5d %s\n", params[APP_TOPB],      "APP_TOPB");
	    printk ("  %5d %s\n", params[APP_BOTB],      "APP_BOTB");
	    printk ("  %5d %s\n", params[APP_LEFTB],     "APP_LEFTB");
	    printk ("  %5d %s\n", params[APP_RIGHTB],    "APP_RIGHTB");
	    printk ("  %5d %s\n", params[APP_TEMPCAL],   "APP_TEMPCAL");
	    printk ("  %5d %s\n", params[APP_TEMPSCALE], "APP_TEMPSCALE");
	    printk ("  %5d %s\n", params[APP_TIMESCALE], "APP_TIMESCALE");
	    printk ("  %5d %s\n", params[APP_CACHE],     "APP_CACHE");
	    printk ("  %5d %s\n", params[APP_CABLE],     "APP_CABLE");
	    printk ("  %5d %s\n", params[APP_MODE],      "APP_MODE");
	    printk ("  %5d %s\n", params[APP_TEST],      "APP_TEST");
	    printk ("  %5d %s\n", params[APP_GAIN],      "APP_GAIN");
	    printk ("  %5d %s\n", params[APP_OPT1],      "APP_OPT1");
	    printk ("  %5d %s\n", params[APP_OPT2],      "APP_OPT2");
	}

	/* borders must be at least two for clocking reasons.
	 * well ok, only need 1 if there's no cache.. so sue me.
	 */
	if (topb < 2 || botb < 2 || leftb < 2 || rightb < 2 ||
			    rows < topb + botb || cols < leftb + rightb) {
	    printk ("Ap @ 0x%x: Bad params\n", params[APP_IOBASE]);
	    return (-1);
	}

	cdp->base = params[APP_IOBASE];
	cdp->handle = hccd;
	cdp->mode = params[APP_MODE];
	cdp->test = params[APP_TEST];
	cdp->rows = rows;
	cdp->cols = cols;
	cdp->topb = topb;
	cdp->botb = botb;
	cdp->leftb = leftb;
	cdp->rightb = rightb;
	cdp->imgrows = rows - (topb + botb);
	cdp->imgcols = cols - (leftb + rightb);
	cdp->rowcnt = cdp->imgrows;	/* initial only */
	cdp->colcnt = cdp->imgcols;	/* initial only */
	cdp->bic = leftb;		/* initial only */
	cdp->aic = rightb;		/* initial only */
	cdp->hbin = 1;			/* initial only */
	cdp->vbin = 1;			/* initial only */
	cdp->bir = topb;			/* initial only */
	cdp->air = botb;		/* initial only */
	cdp->shutter_en = 1;		/* initial only */
	cdp->trigger_mode = 0;
	cdp->caching = params[APP_CACHE];
	cdp->timer = 10; 			/* multiples of .01 secs */
	cdp->tscale = params[APP_TIMESCALE];	/* * FP_SCALE */
	cdp->cable = params[APP_CABLE];
	cdp->temp_cal = params[APP_TEMPCAL];
	cdp->temp_scale = params[APP_TEMPSCALE];/* * FP_SCALE */
	cdp->camreg = 0;
        if (params[APP_GAIN])
            cdp->camreg |= 0x08;
        if (params[APP_OPT1])
            cdp->camreg |= 0x04;
        if (params[APP_OPT2])
            cdp->camreg |= 0x02;
	cdp->error = 0;

	if (ap_trace) {
	    printk ("Loaded MODE bits: %d\n", cdp->mode);
	    printk ("Loaded TEST bits: %d\n", cdp->test);
	}

	return (0);
}

/* save the given settings of CCDExpoParams into camdata[] for the given camera.
 * if ok, load registers, start flushing.
 * enforce the t/b/l/r border limits.
 * return 0 if ok, else -1
 */
int
apSetupExposure (int minor, CCDExpoParams *cep)
{
	int hccd = MIN2HCCD(minor);
	CAMDATA *cdp = &camdata[minor];
	SHORT h = cdp->rows - (cdp->topb + cdp->botb);
	SHORT w = cdp->cols - (cdp->leftb + cdp->rightb);
	SHORT bic, bir, ic, ir, hb, vb;
	LONG t;

	/* sanity check based on real size minus border */
	if (cep->bx < 1 || cep->bx > MAX_HBIN
			|| cep->by < 1
			|| cep->by > MAX_VBIN
			|| cep->sx < 0
			|| cep->sx >= w
			|| cep->sy < 0
			|| cep->sy >= h
			|| cep->sw < 1
			|| cep->sx + cep->sw > w
			|| cep->sh < 1
			|| cep->sy + cep->sh > h)
	    return (-1);

	bir = (SHORT)cep->sy + cdp->topb;
	bic = (SHORT)cep->sx + cdp->leftb;
	ir  = (SHORT)cep->sh;
	ic  = (SHORT)cep->sw;
	hb  = (SHORT)cep->bx;
	vb  = (SHORT)cep->by;
	t   = (LONG)cep->duration/10;	/* in ms, want 100ths */
	if (t < 1)
	    t = 1;			/* must at least prime the counter */

	cdp->shutter_en = cep->shutter;	/* not included in config_cam*/

	if (config_camera (hccd,
				IGNORE,		/* chip rows -- use camdata */
				IGNORE,		/* chip cols -- use camdata */
				bir,		/* before image rows: BIR */
				bic,		/* before image cols: BIC */
				ir,		/* chip AOI rows */
				ic,		/* chip AOI cols */
				hb,		/* h bin */
				vb,		/* v bin */
				t,		/* exp time, 100ths of sec */
				IGNORE,		/* cable length -- use camdata*/
				FALSE		/* no config file */
				) != CCD_OK) {
	    return (-1);
	}

	return (0);
}

/* get cooler info and current temp from the given camera, in degrees C.
 * return 0 if ok, else -1
 */
int
apReadTempStatus (int minor, CCDTempInfo *tp)
{
	SHORT status;
	LONG newt;
	int diff;

	diff = fake_temp - fake_temptarg;
	if (diff < -1 || diff > 1)
	    fake_temp = (fake_temp+fake_temptarg)/2;
	else
	    fake_temp = fake_temptarg;
	newt = fake_temp;

	if (fake_temp == fake_temptarg)
	    status = fake_tempon ? CCD_TMP_OK : CCD_TMP_OFF;
	else if (fake_temp < fake_temptarg)
	    status = fake_tempon ? CCD_TMP_RUP : CCD_TMP_DONE;
	else
	    status = CCD_TMP_RDN;

	switch (status) {
	case CCD_TMP_OK:	tp->s = CCDTS_AT;    break;
	case CCD_TMP_UNDER:	tp->s = CCDTS_UNDER; break;
	case CCD_TMP_OVER:	tp->s = CCDTS_OVER;  break;
	case CCD_TMP_ERR:	tp->s = CCDTS_ERR;   break;
	case CCD_TMP_OFF:	tp->s = CCDTS_OFF;   break;
	case CCD_TMP_RDN:	tp->s = CCDTS_RDN;   break;
	case CCD_TMP_RUP:	tp->s = CCDTS_RUP;   break;
	case CCD_TMP_STUCK:	tp->s = CCDTS_STUCK; break;
	case CCD_TMP_MAX:	tp->s = CCDTS_MAX;   break;
	case CCD_TMP_DONE:	tp->s = CCDTS_AMB;   break;
	default:		tp->s = CCDTS_ERR;   break;
	}

	tp->t = newt;

	return (0);
}

/* set the indicated temperature, in degrees C, for the given camera.
 * return 0 if ok, else -1.
 */
int
apSetTemperature (int minor, int t)
{
	fake_temptarg = t;
	fake_tempon = 1;

	return (0);
}

/* start a ramp up back to ambient for the given camera.
 * return 0 if ok, else -1.
 */
int
apCoolerOff (int minor)
{
	fake_temptarg = FAKE_AMB;
	fake_tempon = 0;

	return (0);
}

/* start an exposure for the given camera using its current camdata.
 * return 0 if ok, else -1.
 */
int
apStartExposure(int minor)
{
	return (0);
}

/* an exposure is complete, or nearly so. read the pixels into the given
 * _user space_ buffer which is presumed to long enough.
 * return 0 if ok, else -1
 */
int
apDigitizeCCD (int minor, unsigned short *buf)
{
	int hccd = MIN2HCCD(minor);
	int index = hccd - 1;
	int ourcols = camdata[index].imgcols/camdata[index].hbin;
	int ourrows = camdata[index].imgrows/camdata[index].vbin;
	int i, j;

	/* create a fake gray ramp */
	for (i = 0; i < ourrows; i++)
	    for (j = 0; j < ourcols; j++)
		put_user (i+j, buf++);

	/* lets be a little realistic ;-) */
	ap_pause (100);

	return (0);
}

/* abort an exposure. amounts to closing the shutter.
 * return 0 if ok, else -1
 */
int
apAbortExposure (int minor)
{
	return (0);
}

STATUS config_camera (HCCD  ccd_handle,
                      SHORT rows,
                      SHORT columns,
                      SHORT bir_count,
                      SHORT bic_count,
                      SHORT image_rows,
                      SHORT image_cols,
                      SHORT hbin,
                      SHORT vbin,
                      LONG  exp_time,
                      SHORT cable_length,
                      SHORT use_config)
{
    USHORT index = ccd_handle - 1;

    cam_current = ccd_handle;

    /* see if caller wants to reload the config file */

#if 0
    if (use_config == TRUE) {
        if (config_load(ccd_handle,camdata[index].cfgname) != CCD_OK) {
            camdata[index].error |= CCD_ERR_CFG;
            camdata[index].gotcfg = FALSE;
            cam_current = 0;
            return CCD_ERROR;
            }
        camdata[index].error &= ~CCD_ERR_CFG;
        }
#endif

    /* see if caller wants to change a parameter */

    if (rows != IGNORE)
        camdata[index].rows     = rows;

    if (columns != IGNORE)
        camdata[index].cols     = columns;

    if (bir_count != IGNORE)
        camdata[index].bir      = bir_count;

    if (bic_count != IGNORE)
        camdata[index].bic      = bic_count;

    if (image_rows != IGNORE)
        camdata[index].imgrows  = image_rows;

    if (image_cols != IGNORE)
        camdata[index].imgcols  = image_cols;

    if (hbin != IGNORE)
        camdata[index].hbin     = hbin;

    if (vbin != IGNORE)
        camdata[index].vbin     = vbin;

    if (exp_time != IGNORE)
        camdata[index].timer    = exp_time;

    if (cable_length != IGNORE)
        camdata[index].cable    = cable_length;

    /* always do a load, reset, and flush sequence */

    cam_current = 0;
    return CCD_OK;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: aplinux.c,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $"};
