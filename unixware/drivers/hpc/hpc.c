/* driver file for the Spectra Source Instruments' HPC-1 CCD camera.
 * the camera-specific code is taken directly from cam215b.{h,c}.
 *
 * this driver supports open/close/ioctl/read/poll:
 * open enforces a single user process using this driver at a time.
 * close closes the shutter and marks the driver as not in use.
 * ioctls set cooler state and save subimage, binning and exposure params and
 *   read the TEC temperature.
 * blocking read causes the exposure to be performed and data to be returned.
 * if the read is interrupted the exposure is aborted and shutter is closed but
 *   reads can not be interrupted during the actual reading of the CCD.
 * non-blocking read (NONBLOCK or NDELAY) just starts the exposure and returns.
 *   remember that UNIX won't even call the driver read entry if count is 0.
 * the size of the read array must be at least large enough to contain the
 *   entire data set from the chip, given the current binning and subimage size.
 * poll(2) is supported and returns ready for POLLS when exposure completes.
 *
 * v1.3  8/5/94 add HPC_READ_TEMP ioctl to read TEC temperature.
 * v1.2  2/4/94	allow exposures of duration 0.
 * v1.1  1/10/94 add shutter field to HPCExpoParams to allow taking Darks.
 * v1.0  1/6/94 get binning right. looking good.
 * v0.2  1/5/94	first trial on real h/w
 * v0.1  1/3/94 start work; Elwood C. Downey
 */

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/open.h>
#include <sys/poll.h>
#include <sys/cred.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/ddi.h>

#include <sys/hpc.h>

/* stuff from space.c */
extern int hpc_io_base;

/* we are obliged to provide this; see devflag(D1D) */
int hpcdevflag = 0;

typedef unsigned short Word;
typedef unsigned short Pixel;

static void hpcWakeup (caddr_t addr);
static void delayMS (int ms);
static void start_exposure ();
static void end_exposure();
static int digitize_ccd (uio_t *uiop);
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

/* opaque structure to support poll(2) system call */
static struct pollhead hpc_ph;

/* current state */
typedef enum {
    HPC_ABSENT,		/* controller not in the computer */
    HPC_CLOSED,		/* controller responding but not in use */
    HPC_OPEN,		/* open for use by user process */
    HPC_EXPOSING,	/* exposing */
    HPC_EXPDONE		/* exposure is compete and ccd may be read */
} HPCState;
static HPCState hpc_state = HPC_ABSENT;	/* until proven otherwise */

/* macros to access the hpc i/o regs that we use.
 * each reg is 16 bits.
 * use these directly with inw and outw.
 */
#define	modePort	(hpc_io_base + 0)
#define	statusPort	(hpc_io_base + 2)
#define	AD1Port		(hpc_io_base + 4)
#define	AD2Port		(hpc_io_base + 6)

/* set by ioctl HPC_SET_EXPO and used by all reads */
static HPCExpoParams hpc_params = {
    2, 2,			/* 2x2 binning */
    0, 0, X_EXTENT, Y_EXTENT,	/* use full chip */
    1000,			/* ms exposure duration */
    1				/* open shutter during exposure */
};

/* sleep priority */
#define	HPC_SLPPRI	(PZERO+1)	/* signalable if > PZERO */

/* the poll(2) state(s) we claim to be "ready" for.
 */
#define	POLLS	(POLLIN|POLLRDNORM)


/* handy yah/nah */
#define	FALSE	0
#define TRUE	1

/* working copy of the mode register */
static Word modeImage = RESTART | (Word) ~TEC_ENABLE_MASK | TEC_INIT_MASK
						| CCD_RESET_MASK | SHUTTER_MASK;

/* timeout id during an exposure.
 * global so we can cancel the timeout if we are interrupted.
 */
static int exp_tid;

/* produces some info on the console if true.
 * you can control this using the HPC_TRACE ioctl.
 */
static int hpctrace = 0;

/* kernel buffer into which we read each row of pixels from the camera
 * when we can't kmem_alloc() anything bigger.
 */
#define	HPCNROWBUF	4	/* rows in the default buffer
				 * N.B. more means more static space here.
				 * N.B. fewer means more calls to delay().
				 */
#define	MAXROWBUFS	8	/* max num rows we ever try to malloc.
				 * N.B. bigger means fewer calls to delay()
				 *    but longer uninterrupted uiomoves.
				 */
#define	UIODELAY	2	/* clock ticks to delay after each uiomove */
static Pixel hpcrowbuf[X_EXTENT*HPCNROWBUF];

/* see whether the HPC appears to be here by seeing if it accepts a RESTART.
 */
void
hpcinit()
{
	cmn_err (CE_CONT, "HPC: addr=0x%x: ", hpc_io_base);

	if (CameraHPCDetect()) {
	    cmn_err (CE_CONT, "ready.\n");
	    hpc_state = HPC_CLOSED;
	} else {
	    cmn_err (CE_CONT, "not found.\n");
	    hpc_state = HPC_ABSENT;
	}
}

/* ARGSUSED */
int
hpcopen (dev_t *devp, int oflag, int otyp, cred_t *crp)
{
	if (hpctrace)
	    cmn_err (CE_CONT, "Open: hpc_state=%d\n", hpc_state);

	switch (hpc_state) {
	case HPC_ABSENT:
	    /* wasn't there when we booted but check again */
	    if (!CameraHPCDetect())
		return (ENODEV);
	    break;
	case HPC_CLOSED:
	    /* recheck camera ok at each open */
	    if (!CameraHPCDetect())
		return (ENODEV);
	    break;
	default:
	    return (EBUSY);
	}

	CameraReset();
	hpc_state = HPC_OPEN;

	return (0);
}

/* clean up any pending timers, stop exposing, close shutter, reset state.
 */
int
hpcclose()
{
	if (hpctrace)
	    cmn_err (CE_CONT, "Close: hpc_state=%d\n", hpc_state);

	if (exp_tid) {
	    untimeout (exp_tid);
	    exp_tid = 0;
	}

	if (hpc_state == HPC_EXPOSING)
	    end_exposure();
	else
	    CameraCloseShutter();
	delayMS (10);

	hpc_state = HPC_CLOSED;

	return (0);
}

/* read data from the hpc, possibly performing an exposure first.
 * pixels are each an unsigned short and the first pixel returned is the upper
 *   left corner.
 * algorithm:
 *   if an exposure has not been started
 *     start an exposure.
 *   if an exposure is in progress and we were called as non-blocking
 *     return 0 or EAGAIN
 *   wait for exposure to complete;
 *   read the chip, copying data to caller;
 * N.B.: if we are ever interrupted while waiting for an exposure, we abort
 *   the exposure, close the shutter immediately and return EINTR.
 * N.B.: the size of the read count must be at least large enough to contain the
 *   entire data set from the chip, given the current binning and subimage size.
 *   if it is not, we return EFAULT.
 */
/* ARGSUSED */
int
hpcread (dev_t dev, uio_t *uiop, cred_t *crp)
{
	int s;

	if (hpctrace)
	    cmn_err (CE_CONT, "Read: hpc_state=%d\n", hpc_state);

	if (hpc_state == HPC_OPEN) {
	    start_exposure();
	    exp_tid = timeout (hpcWakeup, &hpc_params,
				    drv_usectohz(hpc_params.duration*1000));
	    hpc_state = HPC_EXPOSING;
	}

	if (hpc_state == HPC_EXPOSING) {
	    if (uiop->uio_fmode & FNDELAY)
		return (0);
	    if (uiop->uio_fmode & FNONBLOCK)
		return (EAGAIN);	/* POSIX */
	    /* sleep until exposure is complete or there is a signal.
	     * N.B. sleep might return 0 just because the process was
	     *   suspended and resumed, as well as because of wakeup; hence the
	     *   while loop.
	     */
	    while (hpc_state == HPC_EXPOSING) {
		if (sleep (&hpc_params, HPC_SLPPRI|PCATCH)) {
		    untimeout (exp_tid);
		    exp_tid = 0;
		    end_exposure();
		    hpc_state = HPC_OPEN;
		    return (EINTR);
		}
	    }
	    hpc_state = HPC_EXPDONE;
	}

	/* state should be EXPDONE when we get here */
	if (hpc_state != HPC_EXPDONE) {
	    cmn_err (CE_CONT, "hpcread(): state is %d\n", hpc_state);
	    return (EIO);
	}

	s = digitize_ccd (uiop);
	hpc_state = HPC_OPEN;
	return (s);
}

/* ARGSUSED */
int
hpcioctl (dev_t dev, int cmd, void *arg, int mode, cred_t *crp, int *rvalp)
{
	HPCExpoParams hpcp;
	int temperature;

	if (hpctrace)
	    cmn_err (CE_CONT, "Ioctl: cmd=%d\n", cmd);

	switch (cmd) {
	case HPC_SET_EXPO:
	    if (copyin (arg, (caddr_t) &hpcp, sizeof(hpcp)) < 0)
		return (EFAULT);
	    if (hpcp.bx < 0 || hpcp.bx > X_EXTENT
			    || hpcp.by < 0 || hpcp.by > Y_EXTENT
			    || hpcp.sx < 0 || hpcp.sx >= X_EXTENT
			    || hpcp.sy < 0 || hpcp.sy >= Y_EXTENT
			    || hpcp.sw < 1 || hpcp.sx + hpcp.sw > X_EXTENT
			    || hpcp.sh < 1 || hpcp.sy + hpcp.sh > Y_EXTENT)
		return (ENXIO);
	    if (hpcp.bx == 0)
		hpcp.bx = 1;
	    if (hpcp.by == 0)
		hpcp.by = 1;
	    hpc_params = hpcp;
	    if (hpctrace)
		cmn_err (CE_CONT,
		    "Ioctl: sx=%d sy=%d sw=%d sh=%d bx=%d by=%d dur=%d sh=%d\n",
			hpc_params.sx, hpc_params.sy, hpc_params.sw,
			hpc_params.sh, hpc_params.bx, hpc_params.by,
			hpc_params.duration, hpc_params.shutter);
	    break;
	case HPC_SET_COOLER:
	    Cooler ((int)arg);
	    break;
	case HPC_READ_TEMP:
	    temperature = hpc_readTemperature();
	    if (copyout ((caddr_t) &temperature, arg, sizeof(temperature)) < 0)
		return (EFAULT);
	    break;
	case HPC_TRACE:
	    hpctrace = (int)arg;
	    if (hpctrace)
		cmn_err (CE_CONT, "Ioctl: trace on\n");
	    break;
	default:
	    return (EINVAL);
	}

	return (0);
}

/* ARGSUSED */
int
hpcchpoll (dev_t dev, short events, int anyyet, short *revents, struct pollhead **phpp)
{
	if (hpctrace)
	    cmn_err (CE_CONT, "Poll: hpc_state=%d\n", hpc_state);

	if ((events & POLLS) && hpc_state == HPC_EXPDONE) {
	    /* we have stuff to be read */
	    *revents = (events & POLLS);
	} else {
	    /* nothing available to be read */
	    *revents = 0;
	    if (!anyyet)
		*phpp = &hpc_ph;
	}

	return (0);
}

/* the expose timer has expired.
 * end the exposure and wakeup anyone blocking reading or polling.
 */
static void
hpcWakeup (addr)
caddr_t addr;
{
	if (hpctrace)
	    cmn_err (CE_CONT, "Wakeup: hpc_state=%d\n", hpc_state);

	exp_tid = 0;

	if (hpc_state == HPC_EXPOSING) {
	    end_exposure();
	    hpc_state = HPC_EXPDONE;
	    pollwakeup (&hpc_ph, POLLS);
	    wakeup (addr);
	} else
	    cmn_err (CE_CONT, "hpcWakeup(): state is %d\n", hpc_state);
}

/* delay so many ms */
static void
delayMS (ms)
int ms;
{
	delay (drv_usectohz(ms*1000));
}

/* start an exposure.  */
static void
start_exposure ()
{
	if (hpctrace)
	    cmn_err (CE_CONT, "start_exposure: hpc_state=%d\n", hpc_state);

	CameraClearCCD();
	if (hpc_params.shutter)
	    CameraOpenShutter();
	CameraBeginIntegrate();
}

/* terminate a normal exposure */
static void
end_exposure()
{
	if (hpctrace)
	    cmn_err (CE_CONT, "end_exposure: hpc_state=%d\n", hpc_state);

	CameraCloseShutter();
	CameraEndIntegrate();
}

/* read the chip and copy the data up to the caller.
 * take into account binning and subimage.
 * return 0 if ok else an errno.
 * based on DigitizeCCD() in expose.c.
 */
static int
digitize_ccd (uiop)
uio_t *uiop;
{
	int ysize, xsize;	/* net size of image we want, in pixels */
	Pixel *rowbuf;		/* pointer to space for xsize*nrows of pixels */
	int ny;			/* number of rows within rowbuf used so far */
	int nrows;		/* total number of rows within rowbuf */
	int ret;
	int i;

	if (hpctrace)
	    cmn_err (CE_CONT, "digitize_ccd: hpc_state=%d\n", hpc_state);

	/* compute the net number of pixels in each direction */
	ysize = hpc_params.sh/hpc_params.by;
	xsize = hpc_params.sw/hpc_params.bx;

	if (hpctrace)
	    cmn_err (CE_CONT, "    xsize=%d ysize=%d\n", xsize, ysize);

	/* one last size check */
	if (xsize > X_EXTENT) {
	    cmn_err (CE_CONT, "    xsize=%d\n", xsize);
	    return (ENXIO);
	}

	/* try to get a good size buffer to copy into.
	 * if all else fails, use the static buffer hpcrowbuf.
	 */
	rowbuf = NULL;
	for (nrows = min(MAXROWBUFS, ysize); nrows > HPCNROWBUF; nrows /= 2) {
	    rowbuf = kmem_alloc (nrows * xsize * sizeof(Pixel), KM_NOSLEEP);
	    if (rowbuf)
		break;
	}
	if (!rowbuf) {
	    if (hpctrace)
		cmn_err (CE_CONT, "    using static hpcrowbuf\n");
	    rowbuf = hpcrowbuf;
	    nrows = HPCNROWBUF;
	} else {
	    if (hpctrace)
		cmn_err (CE_CONT, "    using malloced %d-row buffer\n", nrows);
	}

	/* throw away the top if we have a starting y location */
	if (hpc_params.sy > 0) {
	    CameraCCDParallel(hpc_params.sy - 1);
	    CameraSkipPixels(xsize + 100);
	    CameraCCDParallel(1);
	}

	/* read each row, copying back to the user process in groups of
	 * nrows for a little more efficiency,
	 */
	for (ret = ny = i = 0; ret == 0 && i < ysize; i++) {
	    CameraDigitizeLine (&rowbuf[xsize*ny], xsize,
				hpc_params.sx, hpc_params.bx, hpc_params.by);
	    if (++ny == nrows || i == ysize-1) {
		/* rowbuf is full or we're done; copy it to user buffer */
		ret = uiomove ((void *)rowbuf, xsize*ny*sizeof(Pixel),
								UIO_READ, uiop);
		/* now we can reuse rowbuf */
		ny = 0;

		/* take a breather; this is very important.
		 * without it we stay in the kernel the whole time which is 
		 * not a very nice timesharing thing to do but worse I've
		 * seen the system spontaneously reboot occasionally.
		 */
		delay (UIODELAY);
	    }
	}

	/* free the row buffer, if we allocated it here */
	if (rowbuf != hpcrowbuf)
	    kmem_free ((void *)rowbuf, nrows * xsize * sizeof(Pixel));

	return (ret);
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
	outw (modePort, modeImage);

	while (soaks-- > 0)
	    (void) inw (statusPort);
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
	outw(modePort, modeImage);
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
	outw (modePort, serial);
	(void) inw (modePort);
	(void) inw (modePort);
	(void) inw (modePort);
	(void) inw (modePort);

	for (i = 0; i < 100; i++) {
	    status = inw (statusPort);		/* read status latch and .. */
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
	if (hpctrace)
	    cmn_err (CE_CONT, "CameraReset: hpc_state=%d\n", hpc_state);

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
	if (hpctrace > 1)
	    cmn_err (CE_CONT, "CameraClearCCD: hpc_state=%d\n", hpc_state);

	CameraCCDParallel(Y_EXTENT*2);	/* shift all up (twice?) */
	CameraSkipPixels(X_EXTENT);	/* then all out */
}

/* skip (discard) count scan line(s). */
static void
CameraCCDParallel(count)
int count;
{
	if (hpctrace > 1)
	    cmn_err (CE_CONT, "CameraCCDParallel: count=%d\n", count);

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
	if (hpctrace)
	    cmn_err (CE_CONT, "CameraCloseShutter: hpc_state=%d\n", hpc_state);

	OutCameraBits (SHUTTER_MASK, 1);
}

static void
CameraOpenShutter()
{
	if (hpctrace)
	    cmn_err (CE_CONT, "CameraOpenShutter: hpc_state=%d\n", hpc_state);

	OutCameraBits (SHUTTER_MASK, 0);
}

/* turns the cooler on or off */
static void
Cooler (on)
int on;
{
	if (hpctrace)
	    cmn_err (CE_CONT, "Cooler: on=%d\n", on);

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
	if (hpctrace)
	    cmn_err (CE_CONT, "CameraBeginIntegrate: hpc_state=%d\n",hpc_state);

	outMode (INTEGRATE, 0);
}

/* start camera integrating */
static void
CameraEndIntegrate()
{
	if (hpctrace)
	    cmn_err (CE_CONT, "CameraEndIntegrate: hpc_state=%d\n",hpc_state);

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
 * pixel offset into linebuf.
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
		    outw (modePort, Hbin2);
		    (void) inw (modePort);
		    outw (modePort, Restart);
		    (void) inw (modePort);
		}
	    }

	    /* digitize the one (net) pixel */
	    outw (modePort, Serial);	/* serial w/reset & digitize */
	    (void) inw (modePort);
	    (void) inw (modePort);
	    (void) inw (modePort);
	    while (inw (statusPort) & 0x40)
		continue;		/* wait for TRK2* true */
	    outw (modePort, Restart);	/* end the present serial */
	    *line++ = inw (AD1Port);	/* read pixel and save it */
	}

	/* drain any remaining pixels from serial register */
	if (numpixels*bx/2+offset < 1090)
	    CameraSkipPixels (1090 - (numpixels*bx/2+offset));

	outw (modePort, Restart);
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
	int i;

        Serial = (modeImage & ~0x3f) | SER2_16;
	Restart = (modeImage & ~0x3f) | RESTART;	/* clock on */
	RestartTEC = Restart & ~TEC_INIT_MASK;		/* clock off */

	/* shift up the odd columns and read them.
	 * N.B. this will fill just the even elements of line[].
	 */

	CCDParallel(OPAR);
	CameraSkipPixels((offset+NUM_DUMMIES)/2);

	outw (modePort, Serial);		/* start an A/D conversion */
	for (i = 0; i < numpixels; i += 2) {
	    (void) inw (modePort);
	    (void) inw (modePort);
	    (void) inw (modePort);
	    (void) inw (modePort);
	    while (inw(statusPort) & 0x40)	/* wait for TK2* true */
		continue;
	    outw (modePort, RestartTEC);
	    (void) inw (modePort);
	    (void) inw (modePort);
	    line[i] = inw (AD1Port);		/* read pixel */
	    outw (modePort, Serial);		/* start next A/D conversion */
	}

	(void) inw (modePort);
	(void) inw (modePort);
	(void) inw (modePort);
	(void) inw (modePort);
	while (inw(statusPort) & 0x40)
	    continue;
	outw (modePort, Restart);

	if (numpixels+offset < 1090)
	    CameraSkipPixels (545 - (numpixels+offset)/2);

	/* now shift up the even columns and start them in the odd locations
	 * if line[]
	 */

	 CCDParallel(EPAR);
	 CameraSkipPixels((offset+NUM_DUMMIES)/2);

	 outw (modePort, Serial);
	 inw (modePort);

	 for (i = 1; i < numpixels; i += 2) {
	    (void) inw (modePort);
	    (void) inw (modePort);
	    (void) inw (modePort);
	    (void) inw (modePort);
	    while (inw(statusPort) & 0x40)
		continue;
	    outw (modePort, RestartTEC);
	    (void) inw (modePort);
	    (void) inw (modePort);
	    line[i] = inw (AD1Port);		/* read pixel */
	    outw (modePort, Serial);		/* start next A/D conversion */
	}

	outw (modePort, Restart);
	modeImage = Restart;

	if(numpixels+offset < 1090)
	    CameraSkipPixels (545 - (numpixels+offset)/2);
}

/* read the current TEC temperature.
 * just read AD2 and return it. mapping must be done by caller.
 */
static int
hpc_readTemperature()
{
	int t;

	t = (int) inw (AD2Port);

	return (t);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: hpc.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
