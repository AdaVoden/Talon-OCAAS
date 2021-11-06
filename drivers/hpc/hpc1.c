/* Linux driver for HPC-1.
 * the camera-specific code is taken directly from cam215b.{h,c}.
 *
 * TODO: complete the work to handle multiple cameras.
 */

#include "hpc.h"

typedef unsigned short Word;
typedef unsigned short Pixel;

/* -- Camera control bits -- */
#define CLOCK_MASK          0x0020
#define TEC_ENABLE_MASK     0x0040
#define TEC_INIT_MASK       0x0080
#define CCD_RESET_MASK      0x0100
#define SHUTTER_MASK        0x0200

#define DEFAULT_BASE_PORT 	0x120

#define RESTART			0x001F
#define SER2_16			0x001E
#define SER2_8			0x001D
#define HBIN1			0x001C
#define HBIN2			0x001B
#define ABG			0x001A
#define INTEGRATE		0x0019

#define OPAR			0x000F
#define EPAR			0x000E
#define FTEPAR			0x000D
#define BPAR			0x000C
#define FTBPAR			0x000B

#define NUM_DUMMIES		24
#define PARALLELPIXELS		80


static void start_exposure (int minor);
static void end_exposure(int minor);
static int digitize_ccd (int minor, unsigned short *userbuf);
static void outMode (unsigned modeWord, int soaks);
static void OutCameraBits(unsigned mask, int on);
static void OutCameraBits(unsigned mask, int on);
static int CameraHPCDetect();
static void CameraReset();
static void CameraClearCCD();
static void CameraCCDParallel(int count);
static void CameraSkipPixels(int count);
static void CameraCloseShutter();
static void CameraOpenShutter();
static void Cooler (int on);
static void CameraBeginIntegrate();
static void CameraEndIntegrate();
static void CameraDigitizeLine (Pixel *linebuf, int numpixels, int offset,
    int bx, int by);
static void CCDParallel (int op);
static void DigitizeLineHiResBinning (Pixel *line, int numpixels, int offset,
    int bx, int by);
static void DigitizeLineHiRes (Pixel *line, int numpixels, int offset);
static int hpc_readTemperature(void);


static CCDExpoParams hpc_params[MAXCAMS];	/* exposure setup */
static CCDTempInfo hpc_tempinfo[MAXCAMS];	/* goal temp */

/* macros to access the hpc i/o regs that we use.
 * each reg is 16 bits.
 * use these directly with inpw and outpw.
 */
static int hpc_io_base;	/* base address */
#define	modePort	(hpc_io_base + 0)
#define	statusPort	(hpc_io_base + 2)
#define	AD1Port		(hpc_io_base + 4)
#define	AD2Port		(hpc_io_base + 6)

/* handy yah/nah */
#undef FALSE
#define	FALSE	0
#undef TRUE	1
#define TRUE	1

/* working copy of the mode register */
static Word modeImage = RESTART | (Word) ~TEC_ENABLE_MASK | TEC_INIT_MASK
						| CCD_RESET_MASK | SHUTTER_MASK;


/* see whether the HPC appears to be here by seeing if it accepts a RESTART.
 * return 1 if appears to be present, else return 0
 */
int
hpcProbe(int base)
{
	hpc_io_base = base;
	return (CameraHPCDetect());
}

/* one-time camera setup.
 * return 0 if ok, else -1
 */
int
hpcInit(int minor, int params[])
{
	CCDExpoParams *cep = &hpc_params[minor];
	CCDTempInfo *htp = &hpc_tempinfo[minor];

	/* sensible defaults */
	cep->bx = cep->by = 1;
	cep->sx = cep->sy = 0;
	cep->sw = X_EXTENT;
	cep->sh = Y_EXTENT;
	cep->duration = 1000;
	cep->shutter = 1;

	CameraReset();
	Cooler (1);
	htp->t = -30;			/* ?? */
	htp->s = CCDTS_RDN;		/* ?? */

	return (0);
}

/* per-open init.
 * return 0 if ok, else -1
 */
int
hpcReset (int minor)
{
	CameraReset();
	return (0);
}

/* save the given settings of CCDExpoParams into hpc_params[minor].
 * return 0 if ok, else -1
 */
int
hpcSetupExposure (int minor, CCDExpoParams *cep)
{
	if (cep->bx < 0 || cep->bx > X_EXTENT
			|| cep->by < 0 || cep->by > Y_EXTENT
			|| cep->sx < 0 || cep->sx >= X_EXTENT
			|| cep->sy < 0 || cep->sy >= Y_EXTENT
			|| cep->sw < 1 || cep->sx + cep->sw > X_EXTENT
			|| cep->sh < 1 || cep->sy + cep->sh > Y_EXTENT)
	    return (-1);

	if (cep->bx == 0)
	    cep->bx = 1;
	if (cep->by == 0)
	    cep->by = 1;

	hpc_params[minor] = *cep;

	if (hpc_trace)
	    printk ("HPC: sx=%d sy=%d sw=%d sh=%d bx=%d by=%d dur=%d sh=%d\n",
		    cep->sx, cep->sy, cep->sw,
		    cep->sh, cep->bx, cep->by,
		    cep->duration, cep->shutter);

	return (0);
}

/* get cooler info and current temp from the given camera, in degrees C.
 * return 0 if ok, else -1
 */
int
hpcReadTempStatus (int minor, CCDTempInfo *tp)
{
	CCDTempInfo *htp = &hpc_tempinfo[minor];

	if (htp->s == CCDTS_OFF)
	    tp->s = CCDTS_OFF;
	else {
	    /* compare current with target to decide state */
	    tp->t = hpc_readTemperature();
	    if (tp->t > htp->t)
		tp->s = CCDTS_RDN;
	    else
		tp->s = CCDTS_AT;	/* ?? */
	}

	return (0);
}

/* return 0 if ok, else -1.
 */
int
hpcSetTemperature (int minor, int t)
{
	CCDTempInfo *htp = &hpc_tempinfo[minor];

	htp->s = CCDTS_RDN;
	htp->t = t;	/* store as a target */
	Cooler (1);
	return (0);
}

/* return 0 if ok, else -1.
 */
int
hpcCoolerOff (int minor)
{
	CCDTempInfo *htp = &hpc_tempinfo[minor];

	htp->s = CCDTS_OFF;
	Cooler (0);
	return (0);
}

/* start an exposure for the given camera using its current camdata.
 * return 0 if ok, else -1.
 */
int
hpcStartExposure(int minor)
{
	start_exposure (minor);
	return (0);
}

/* an exposure is complete, or nearly so. read the pixels into the given
 * _user space_ buffer which is presumed to be long enough.
 * return 0 if ok, else -1
 */
int
hpcDigitizeCCD (int minor, unsigned short *buf)
{
	end_exposure(minor);
	return (digitize_ccd (minor, buf));
}

/* abort an exposure in progress.
 * return 0 if ok, else -1
 */
int
hpcAbortExposure (int minor)
{
	CameraCloseShutter();
        CameraEndIntegrate();

	return (0);
}


/* implementation details */



/* start an exposure.  */
static void
start_exposure (int minor)
{
	if (hpc_trace)
	    printk ("HPC start_exposure\n");

	CameraClearCCD();
	if (hpc_params[minor].shutter)
	    CameraOpenShutter();
	CameraBeginIntegrate();
}

/* terminate a normal exposure */
static void
end_exposure(int minor)
{
	if (hpc_trace)
	    printk ("HPC end_exposure\n");

	CameraCloseShutter();
	CameraEndIntegrate();
}

/* read the chip and copy the data up to the caller.
 * take into account binning and subimage.
 * return 0 if ok else -1
 * based on DigitizeCCD() in expose.c.
 * N.B. usebuf is _user space_ buffer which is presumed to be long enough.
 */
static int
digitize_ccd (int minor, unsigned short *userbuf)
{
	CCDExpoParams *cep = &hpc_params[minor];
	int ysize, xsize;	/* net size of image we want, in pixels */
	int i;

	if (hpc_trace)
	    printk ("HPC digitize_ccd\n");

	/* compute the net number of pixels in each direction */
	xsize = cep->sw/cep->bx;
	ysize = cep->sh/cep->by;

	if (hpc_trace)
	    printk ("    xsize=%d ysize=%d\n", xsize, ysize);

	/* one last size check */
	if (xsize > X_EXTENT) {
	    printk ("    xsize=%d\n", xsize);
	    return (-1);
	}

	/* throw away the top if we have a starting y location */
	if (cep->sy > 0) {
	    CameraCCDParallel(cep->sy - 1);
	    CameraSkipPixels(xsize + 100);
	    CameraCCDParallel(1);
	}

	/* read each row, copying back to the user process */
	for (i = 0; i < ysize; i++) {
	    CameraDigitizeLine (&userbuf[i*xsize], xsize,
						    cep->sx, cep->bx, cep->by);
	    /* take a breather */
	    if ((i % hpc_impact) == 0)
		hpc_pause(1);
	}

	return (0);
}

/* set the control portion of the mode register modeImage to modeWord;
 * load it into the modePort;
 * then read the statusPort soaks times.
 */
static void
outMode (modeWord, soaks)
unsigned modeWord;
int soaks;
{
	modeImage &= ~0x3f;
	modeImage |= (modeWord & 0x3f);
	outpw (modePort, modeImage);

	while (soaks-- > 0)
	    (void) inpw (statusPort);
}

/* if on, `or' mask with modeImage, else `and' it with ~mask.
 * then send modeImage to the modePort port.
 */
static void
OutCameraBits(mask, on)
unsigned mask;
int on;
{
	if(on)
	    modeImage |= mask;
	else
	    modeImage &= ~mask;
	outpw(modePort, modeImage);
}

/*  Check existence of HPC-1 type camera at the current port.
 *  returns TRUE if camera found, FALSE if not.
 */
static int
CameraHPCDetect()
{
	Word serial, status;
	int i;

	outMode((modeImage & ~0x3f) | RESTART, 4);
	serial = (modeImage & ~0x3f) | SER2_16;
	outpw (modePort, serial);
	(void) inpw (modePort);
	(void) inpw (modePort);
	(void) inpw (modePort);
	(void) inpw (modePort);

	for (i = 0; i < 100; i++) {
	    status = inpw (statusPort);		/* read status latch and .. */
	    if ((status & 0x40) == 0) {		/* test for TRK true */
		outMode (RESTART, 4);
		return (TRUE);
	    }
	}

	return (FALSE);
}

/* reset the camera */
static void
CameraReset()
{
	if (hpc_trace)
	    printk ("HPC CameraReset\n");

        CameraCloseShutter();
	CameraClearCCD();
	CameraClearCCD();
	CameraClearCCD();
	CameraClearCCD();

	OutCameraBits(TEC_INIT_MASK, FALSE);            /* strobe TEC init */
	OutCameraBits(TEC_INIT_MASK, TRUE);

	outMode(RESTART, 4);
}

/* clear out the ccd */
static void
CameraClearCCD()
{
	if (hpc_trace > 1)
	    printk ("HPC CameraClearCCD\n");

	CameraCCDParallel(Y_EXTENT*2);	/* shift all up (twice?) */
	CameraSkipPixels(X_EXTENT);	/* then all out */
}

/* skip (discard) count scan line(s). */
static void
CameraCCDParallel(count)
int count;
{
	if (hpc_trace > 1)
	    printk ("HPC CameraCCDParallel: count=%d\n", count);

	while (count-- > 0) {
	    CCDParallel (EPAR);
	}
}

/* skip (discard) count pixels in the serial regsiter */
static void
CameraSkipPixels(count)
int count;
{
	while (count-- > 0) {
	    outMode (SER2_8, 7);		/* fast serial shift */
	    outMode (RESTART, 3);
	}
}

static void
CameraCloseShutter()
{
	if (hpc_trace)
	    printk ("HPC CameraCloseShutter\n");

	OutCameraBits (SHUTTER_MASK, 1);
}

static void
CameraOpenShutter()
{
	if (hpc_trace)
	    printk ("HPC CameraOpenShutter\n");

	OutCameraBits (SHUTTER_MASK, 0);
}

/* turns the cooler on or off */
static void
Cooler (on)
int on;
{
	if (hpc_trace)
	    printk ("HPC Cooler: on=%d\n", on);

	if(on) {
	    OutCameraBits(TEC_ENABLE_MASK, FALSE);
	    OutCameraBits(TEC_INIT_MASK, FALSE);	/* strobe TEC init */
	    OutCameraBits(TEC_INIT_MASK, TRUE);
	} else {
	    OutCameraBits(TEC_ENABLE_MASK, TRUE);
	    OutCameraBits(TEC_INIT_MASK, FALSE);	/* strobe TEC init */
	    OutCameraBits(TEC_INIT_MASK, TRUE);
	}
}

/* start camera integrating */
static void
CameraBeginIntegrate()
{
	if (hpc_trace)
	    printk ("HPC CameraBeginIntegrate\n");

	outMode (INTEGRATE, 0);
}

/* start camera integrating */
static void
CameraEndIntegrate()
{
	if (hpc_trace)
	    printk ("HPC CameraEndIntegrate\n");

	outMode (RESTART, 0);
}

/* do one parallel operation of the given type */
static void
CCDParallel (op)
int op;
{
	if (op == HBIN1) {
	    outMode (op, 2);	/* TODO: 2 is ParallelDelay config param */
	    outMode (RESTART, 2);
	    outMode (OPAR, 2);	/* TODO: 2 is ParallelDelay config param */
	    outMode (RESTART, 2);
	} else {
	    outMode (op, 2);
	    outMode (RESTART, 2);
	}
}

/* digitize the next by lines of (numpixels*bx) pixels beginning at the given
 * pixel offset into line.
 * line[] contains room for numpixels Pixel; ie, numpixels if the number of
 *  _net_ pixels after allowing for X binning.
 */
static void
CameraDigitizeLine (line, numpixels, offset, bx, by)
Pixel *line;
int numpixels;
int offset;
int bx, by;
{
	if (bx > 1 || by > 1)
	    DigitizeLineHiResBinning (line, numpixels, offset, bx, by);
	else
	    DigitizeLineHiRes (line, numpixels, offset);
}

/* read a row of pixels into line[], allowing for binning.
 * line[] contains room for numpixels Pixel.
 */
static void
DigitizeLineHiResBinning (line, numpixels, offset, bx, by)
Pixel *line;	/* pointer to line buffer */
int numpixels;	/* number of pixels to digitize, after binning */
int offset;	/* pixel offset into line */
int bx, by;	/* binning factors */
{
	Word Serial, Restart, Hbin2;
	Pixel p;
	int nmore;
	int i, j;

	Serial = (modeImage & ~0x3f) | SER2_16;	/* serial with reset and digz */
	Restart = (modeImage & ~0x3f) | RESTART;
	Hbin2 = (modeImage & ~0x3f) | HBIN2;

	/* bin rows together */
	while (by-- > 0)
	    CCDParallel (BPAR);

	/* if no x binning, we can't just get by with the odd columns
	 * like we do from here on
	 */
	if (bx == 1) {
	    DigitizeLineHiRes (line, numpixels, offset);
	    return;
	}

	/* odd + even mux vertical operation.
	 * this does one x bin already.
	 */
	CCDParallel (HBIN1);

	CameraSkipPixels ((offset+NUM_DUMMIES)/2);

	for (i = 0; i < numpixels; i++) {
	    nmore = bx/2;
	    if (nmore > 1) {
		/* add in the extra x binned pixels */
		for (j = 0; j < nmore-1; j++) {
		    outpw (modePort, Hbin2);
		    (void) inpw (modePort);
		    outpw (modePort, Restart);
		    (void) inpw (modePort);
		}
	    }

	    /* digitize the one (net) pixel */
	    outpw (modePort, Serial);	/* serial w/reset & digitize */
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    while (inpw (statusPort) & 0x40)
		continue;		/* wait for TRK2* true */
	    outpw (modePort, Restart);	/* end the present serial */
	    p = inpw (AD1Port);		/* read pixel and .. */
	    put_user (p, line);		/* pass to user */
	    line++;
	}

	/* drain any remaining pixels from serial register */
	if (numpixels*bx/2+offset < 1090)
	    CameraSkipPixels (1090 - (numpixels*bx/2+offset));

	outpw (modePort, Restart);
	modeImage = Restart;
}

/* read a row of pixels into line[].
 * line[] contains room for numpixels Pixel.
 * N.B. we don't allow for binning here at all.
 */
static void
DigitizeLineHiRes (line, numpixels, offset)
Pixel *line;	/* pointer to line buffer */
int numpixels;	/* number of pixels to digitize, after binning */
int offset;	/* pixel offset into line */
{
	Word Serial, Restart, RestartTEC;
	Pixel p;
	int i;

        Serial = (modeImage & ~0x3f) | SER2_16;
	Restart = (modeImage & ~0x3f) | RESTART;	/* clock on */
	RestartTEC = Restart & ~TEC_INIT_MASK;		/* clock off */

	/* shift up the odd columns and read them.
	 * N.B. this will fill just the even elements of line[].
	 */

	CCDParallel(OPAR);
	CameraSkipPixels((offset+NUM_DUMMIES)/2);

	outpw (modePort, Serial);		/* start an A/D conversion */
	for (i = 0; i < numpixels; i += 2) {
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    while (inpw(statusPort) & 0x40)	/* wait for TK2* true */
		continue;
	    outpw (modePort, RestartTEC);
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    p = inpw (AD1Port);			/* read pixel and .. */
	    put_user (p, &line[i]);		/* pass to user */
	    outpw (modePort, Serial);		/* start next A/D conversion */
	}

	(void) inpw (modePort);
	(void) inpw (modePort);
	(void) inpw (modePort);
	(void) inpw (modePort);
	while (inpw(statusPort) & 0x40)
	    continue;
	outpw (modePort, Restart);

	if (numpixels+offset < 1090)
	    CameraSkipPixels (545 - (numpixels+offset)/2);

	/* now shift up the even columns and start them in the odd locations
	 * if line[]
	 */

	CCDParallel(EPAR);
	CameraSkipPixels((offset+NUM_DUMMIES)/2);

	outpw (modePort, Serial);
	inpw (modePort);

	for (i = 1; i < numpixels; i += 2) {
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    while (inpw(statusPort) & 0x40)
		continue;
	    outpw (modePort, RestartTEC);
	    (void) inpw (modePort);
	    (void) inpw (modePort);
	    p = inpw (AD1Port);			/* read pixel and .. */
	    put_user (p, &line[i]);		/* pass to user */
	    outpw (modePort, Serial);		/* start next A/D conversion */
	}

	outpw (modePort, Restart);
	modeImage = Restart;

	if(numpixels+offset < 1090)
	    CameraSkipPixels (545 - (numpixels+offset)/2);
}

/* read the current TEC temperature, return in degrees C.
 */
static int
hpc_readTemperature()
{
	int raw = ((int)inpw (AD2Port))&0xff;
	/* this was just nuts
	int degk = raw/2 + 200;
	*/
	int degk = raw*2 - 230;
	int degc = degk - 273;

	if (hpc_trace)
	    printk ("HPC temperature: raw=%d degc=%d\n", raw&0xff, degc);

	return (degc);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: hpc1.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
