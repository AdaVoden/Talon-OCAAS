/* Linux driver (module actually) to simulate a dummy camera.
 * (C) Copyright 1998 Elwood Charles Downey. All rights reserved.
 *
 *   22 Jan 98  Begin work -- hacked on apogee driver
 *
 * To build and install:
 *   cc -Wall -O -DAP_MJR=120 -DMODULE -D__KERNEL__ -c *.c   # compile each .c
 *   ld -r -o apogee.o *.o			# link into one .o
 *   mknod apogee c 120 0	        	# make a special file
 *   insmod apogee.o apg0=1,2,3.. apg1=4,5,6..	# install module (see below)
 *
 * This code is based directly on Apogee's API source code, by permission.
 *
 * There are no interrupts. The I/O base addresses are set via module params.
 *
 * This driver supports open/close/ioctl/read/select as follows:
 *
 *   open() enforces a single user process using this driver at a time. If
 *     the mode flags are RDWR we actually try connecting to the board. If
 *     the mode flags are just WRONLY, we just read in and save a config file.
 *
 *   close() aborts an exposure in progress, if any, and marks the driver as
 *     not in use.
 *
 *   ioctl()'s do various functions as follows:
 *     CCD_SET_EXPO: set desired exposure duration, subimage, binning, shutter
 *     CCD_SET_TEMP: set cooler temp in degrees C, or turn cooler off if >= 0
 *     CCD_GET_TEMP: read the TEC temperature, in degrees C
 *     CCD_GET_SIZE: get chip size
 *     CCD_GET_ID:   get an ident string for camera
 *     CCD_GET_DUMP: get register dump for debugging
 *     CCD_TRACE:    turn tracing on or off
 *
 *   read() performs exposures and returns pixels. the exposure parameters are
 *     those supplied by the most recent CCD_SET_EXPO ioctl.
 *   blocking read:
 *     if there is no exposure in progress, a blocking read opens the shutter,
 *       waits for the end of the exposure, closes the shutter, reads the pixels
 *       back to the caller and returns. if the read is signaled while waiting,
 *       the exposure is aborted, the shutter is closed and errno=EINTR.
 *     if there is an exposure in progress, a blocking read will wait for it
 *       to complete, then close the shutter and return the pixels.
 *   non-blocking read (NONBLOCK):
 *     if there is no exposure in progress, a non-blocking read just
 *       opens the shutter, starts the exposure and returns 0 immediately
 *       without returning any data. the count given in the read call is
 *       immaterial, and the buffer is not used at all, but you should give a
 *       count > 0 just to insure the driver is really called. the shutter will
 *       be closed at the end of the exposure even if no process is listening
 *       at the time but beware that the pixels will continue to accumulate
 *       dark current until they are read out.
 *     if there is an exposure in progress, a non-blocking read returns -1 with
 *       errno=EAGAIN.
 *   if an exposure has already completed but the pixels have not yet been
 *       retrieved, either flavor of read will return the pixels immediately.
 *   N.B.: partial reads are not supported, ie, reads that return data must
 *     provide an array of at least sw/bx*sh/by*2 bytes or else all pixels
 *     are lost and read returns -1 with errno=EFAULT.
 *
 *   select() will indicate a read will not block when the exposure is complete.
 *
 *   So: using a blocking read is the easiest way to run the camera, if your
 *   process has nothing else to do. But if it does (such as an X client), then
 *   start the exposure with a non-blocking read, use select() to check, then
 *   read the pixels as soon as select says the read won't block.
 *
 * When installing with insmod, the camera parameters must be set using apg0
 *   for the first camera, etc, as below. Also, ap_impact may be set to
 *   adjust the degree to which reading pixels impacts other system activities.
 *   1 is minimal impact, higher numbers have greater impact but result in
 *   better images. If your images consistently have horizontal streaks in
 *   them, you need a higher number. Go gradually and make it no larger than
 *   necessary.
 *
 * TODO: finish support for multiple devices. All the supporting functions
 *  already accept minor device number. We just need to make the state and
 *  timers arrays here.
 */

#include <linux/version.h>
#include <linux/module.h>

#include "apglob.h"			/* Apogee globals */
#include "aplinux.h"			/* linux glue */

#define AP_MAJVER	1		/* major version number */
#define AP_MINVER	0		/* minor version number */

/* user-defineable options: */

static char ap_name[] = "fakecam";	/* name known to module tools */
static char ap_id[] = "Apogee CCD Camera"; /* id string */

/* The following arrays hold values which are set when the module is loaded
 * via insmod. Many values may be discerned directly from the .ini file
 * supplied by Apogee. Values which are 0 may be skipped. The border values
 * are to allow the driver to hide edge cosmetics from the applications. The
 * only real restriction on them is that they be at least two. For example, with
 * the AP7 camera on a long cable:
 *
 *   insmod apogee.o apg0=0x290,520,520,4,4,4,4,100,210,100,0,1,0,0,1
 *
 *   .ini name		Description
 *   ---------		-----------
 *   base		base address, in hex with leading 0x
 *   rows		total number of actual rows on chip
 *   columns		total number of actual columns on chip
 *   (top border)	top rows to be hidden, >= 2
 *   (bottom border)	bottom rows to be hidden, >= 2
 *   (left border)	left columns to be hidden, >= 2
 *   (right border)	right columns to be hidden, >= 2
 *   cal		temp calibration, as per .ini file
 *   scale		temp scale, as per .ini file but x100
 *   tscale		time scale, as per .ini file but x100
 *   caching		1 if controller supports caching, else 0
 *   cable		1 if camera is on a long cable, 0 if short
 *   mode		mode bits, as per .ini file (optional)
 *   test		test bits, as per .ini file (optional)
 *   16bit		1 if camera has 16 bit pixels, 0 if 12 or 14
 *   gain		CCD gain option (unused)
 *   opt1		Factory defined option 1 (unused)
 *   opt2		Factory defined option 2 (unused)
 */

/* number of arrays should match MAXCAMS.
 * would make it a 2d array if knew how to init it from insmod :-(
 */
static int apg0[APP_N];
static int apg1[APP_N];
static int apg2[APP_N];
static int apg3[APP_N];
static int *apg[MAXCAMS] = {apg0, apg1, apg2, apg3};

/* end of user-defineable options */

int ap_trace;				/* set for printk traces; > for more */
int ap_impact;				/* pixel reading impact; see apapi.c */
static int ap_nfound;			/* how many cameras are connected */

#define	AP_IO_EXTENT	16		/* bytes of I/O space */
#define	AP_SELTO	(20*HZ/100)	/* select() polling rate, jiffies */

/* driver state */
typedef enum {
    AP_ABSENT,				/* controller not known to be present */
    AP_CLOSED,				/* installed, but not now in use */
    AP_OPEN,				/* open for full use, but idle */
    AP_EXPOSING,			/* taking an image */
    AP_EXPDONE				/* image ready to be read */
} APState;

/* all of these are indexed by minor device number */
static CCDExpoParams ap_cep[MAXCAMS];	/* user's exposure details */
static APState ap_state[MAXCAMS];	/* current state of driver */
static struct wait_queue *ap_wq[MAXCAMS];/* expose-completion wait q */
static struct timer_list ap_tl[MAXCAMS];/* expose-completion timer list */
static int ap_timeron[MAXCAMS];		/* set when expose timer is on */
static struct wait_queue *ap_swq[MAXCAMS];/* used for select() */
static struct timer_list ap_stl[MAXCAMS];/* used for select() */
static int ap_stimeron[MAXCAMS];	/* set when select timer is on */

/* parameters to implement a running average temperature */
#define	RATEMPTO	(3000*HZ/100)	/* max ratemp age, jiffies */
#define	NRATEMP		10		/* max nratemp */
static int ap_ratemp[NRATEMP];		/* history */
static int ap_nratemp;			/* N readings contributing to ratemp */
static int ap_ratemp_lj;		/* jiffies at last temp reading */

/* handy way to pause the current process for n jiffies.
 * N.B. do _not_ use this with select().
 */
void
ap_pause (int n)
{
	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + n;
	schedule();
}

/* exposure is complete -- wake up ap_wq */
static void
expComplete (unsigned long minor)
{
	if (ap_trace)
	    printk ("expComplete()\n");
	ap_state[minor] = AP_EXPDONE;
	ap_timeron[minor] = 0;
	wake_up_interruptible (&ap_wq[minor]);
}

/* sleep until the exposure is complete, as indicated by a wakeup on ap_wq. */
static void
waitForExpTimer(int minor)
{
	if (ap_trace)
	    printk ("waitForExpTimer()\n");
	interruptible_sleep_on (&ap_wq[minor]);
}

/* turn off any pending exposure timer, if any. */
static void
stopExpTimer(int minor)
{
	if (ap_timeron[minor]) {
	    del_timer (&ap_tl[minor]);
	    ap_timeron[minor] = 0;
	}
}

/* start a fresh timer for the duration of the exposure */
static void
startExpTimer(int minor)
{
	if (ap_trace)
	    printk ("startExpTimer()\n");
	stopExpTimer(minor);
	init_timer (&ap_tl[minor]);
	ap_tl[minor].expires = jiffies + ap_cep[minor].duration*HZ/1000 + 1;
	ap_tl[minor].function = expComplete;	/* will wakeup ap_wq */
	ap_tl[minor].data = minor;
	add_timer (&ap_tl[minor]);
	ap_timeron[minor] = 1;
}

/* turn off any pending select() timer, if any. */
static void
stopSelectTimer(int minor)
{
	if (ap_stimeron[minor]) {
	    del_timer (&ap_stl[minor]);
	    ap_stimeron[minor] = 0;
	}
}

/* called to start another wait/poll interval on behalf of select() */
static void
startSelectTimer (int minor)
{
	if (ap_trace > 1)
	    printk ("startSelectTimer()\n");
	stopSelectTimer(minor);
	init_timer (&ap_stl[minor]);
	ap_stl[minor].expires = jiffies + AP_SELTO;
	ap_stl[minor].function = (void(*)(unsigned long))wake_up_interruptible;
	ap_stl[minor].data = (unsigned long) &ap_swq[minor];
	add_timer (&ap_stl[minor]);
	ap_stimeron[minor] = 1;
}

/* get cooler state and running-ave temperature, in C.
 * return 0 if ok, else -1
 */
static int
ap_ReadRATempStatus (int minor, CCDTempInfo *tp)
{
	int sum;
	int i;

	/* read the real temp now */
	if (apReadTempStatus (minor, tp) < 0)
	    return (-1);

	/* discard running average if too old */
	if (jiffies > ap_ratemp_lj + RATEMPTO)
	    ap_nratemp = 0;
	ap_ratemp_lj = jiffies;

	/* insert at 0, shifting out first to make room */
	for (i = ap_nratemp; --i > 0; )
	    ap_ratemp[i] = ap_ratemp[i-1];
	ap_ratemp[0] = tp->t;
	if (ap_nratemp < NRATEMP)
	    ap_nratemp++;

	/* compute new average and update */
	for (i = sum = 0; i < ap_nratemp; i++)
	    sum += ap_ratemp[i];

	/* like floor(sum/ap_nratemp + 0.5) */
	tp->t = (2*(sum+1000)/ap_nratemp + 1)/2 - 1000/ap_nratemp;
	return (0);
}

/* open the device and return 0, else a errno.
 */
static int
ap_open (struct inode *inode, struct file *file)
{
	int minor = MINOR (inode->i_rdev);
	int flags = file->f_flags;

	if (ap_trace)
	    printk ("ap_open(%d): ap_state=%d\n", minor, ap_state[minor]);
	if (minor < 0 || minor >= MAXCAMS)
	    return (-ENODEV);
	if (ap_state[minor] == AP_ABSENT)
	    return (-ENODEV);
	if (ap_state[minor] != AP_CLOSED)
	    return (-EBUSY);

	/* there's no write but RDWR captures the spirit of ioctl. */
	if ((flags & O_ACCMODE) != O_RDWR)
	    return (-EACCES);

	ap_state[minor] = AP_OPEN;
	MOD_INC_USE_COUNT;
	return (0);
}    

/* called when driver is closed. clean up and decrement module use count */
static void
ap_release (struct inode *inode, struct file *file)
{
	int minor = MINOR (inode->i_rdev);

	if (ap_trace)
	    printk ("ap_release()\n");
	stopExpTimer(minor);
	stopSelectTimer(minor);
	if (ap_state[minor] == AP_EXPOSING)
	    apAbortExposure(minor);
	ap_state[minor] = AP_CLOSED;
	MOD_DEC_USE_COUNT;
}    

/* fake system call for lseek -- just so cat works */
static int
ap_lseek (struct inode *inode, struct file *file, off_t offset, int orig)
{
	if (ap_trace)
	    printk ("ap_lseek()\n");
	return (file->f_pos = 0);
}

/* read data from the camera, possibly performing an exposure first.
 * pixels are each an unsigned short and the first pixel returned is the upper
 *   left corner.
 *
 * algorithm:
 *   sanity check incoming buffer, if it might be used
 *   if an exposure has not been started
 *     start an exposure.
 *     if called as non-blocking
 *       that's all we do -- return 0
 *   if an exposure is in progress
 *     if we were called as non-blocking
 *       return EAGAIN
 *     wait for exposure to complete;
 *   data is ready: read the chip, copying data to caller, returning count;
 *
 * N.B. if we are ever interrupted while waiting for an exposure, we abort
 *   the exposure, close the shutter and return EINTR.
 * N.B. the size of the read count must be at least large enough to contain the
 *   entire data set from the chip, given the current binning and subimage size.
 *   if it is not, we return EFAULT.
 */
static int
ap_read (struct inode *inode, struct file *file, char *buf, int count)
{
	int minor = MINOR (inode->i_rdev);
	CCDExpoParams *cep = &ap_cep[minor];
	int nbytes = cep->sw/cep->bx * cep->sh/cep->by * 2;
	int s;

	if (ap_trace)
	    printk ("ap_read() ap_state=%d\n", ap_state[minor]);

	/* sanity check buf if we are really going to use it */
	if (ap_state[minor] == AP_EXPDONE || !(file->f_flags & O_NONBLOCK)) {
	    if (((unsigned)buf) & 1)
		return (-EFAULT);
	    s = verify_area(VERIFY_WRITE, (const void *) buf, nbytes);
	    if (s)
		return (s);
	}

	if (ap_state[minor] == AP_OPEN) {
	    if (apStartExposure(minor) < 0)
		return (-ENXIO);
	    startExpTimer(minor);
	    ap_state[minor] = AP_EXPOSING;
            if (file->f_flags & O_NONBLOCK)
		return (0);
	}

	if (ap_state[minor] == AP_EXPOSING) {
            if (file->f_flags & O_NONBLOCK)
		return (-EAGAIN);
	    waitForExpTimer(minor);
	    if (ap_state[minor] == AP_EXPOSING) {
		/* still exposing means sleep was interrupted -- abort */
		stopExpTimer(minor);
		apAbortExposure (minor);
		ap_state[minor] = AP_OPEN;
		return (-EINTR);
	    }
	    ap_state[minor] = AP_EXPDONE;
	}

	if (ap_state[minor] != AP_EXPDONE) {
	    printk ("%s[%d]: ap_read but state is %d\n", ap_name, minor,
							    ap_state[minor]);
	    ap_state[minor] = AP_OPEN;
	    return (-EIO);
	}

	s = apDigitizeCCD (minor, (unsigned short *)buf);
	ap_state[minor] = AP_OPEN;
	return (s < 0 ? -EPERM : nbytes);
}

/* system call interface for ioctl.
 * we support setting exposure params, turning the cooler on and off, reading
 * the camera temperature.
 */
static int
ap_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
unsigned long arg)
{
	int minor = MINOR (inode->i_rdev);
	CAMDATA *cdp = &camdata[minor];
	char id[sizeof(ap_id) + 10];
	CCDExpoParams cep;
	CCDTempInfo tinfo;
	int s;

	switch (cmd) {
	case CCD_SET_EXPO: /* save caller's CCDExpoParams, at arg */
	    s = verify_area(VERIFY_READ, (void *) arg, sizeof(cep));
	    if (s)
		return (s);
	    memcpy_fromfs((void *)&cep, (const void *)arg, sizeof(cep));

	    if (ap_trace)
		printk ("ExpoParams: %d/%d %d/%d/%d/%d %d ms\n", cep.bx, cep.by,
				cep.sx, cep.sy, cep.sw, cep.sh, cep.duration);

	    if (cep.duration < 0 || apSetupExposure (minor, &cep) < 0)
		return (-ENXIO);

	    ap_cep[minor] = cep;
	    break;

	case CCD_SET_TEMP: /* set temperature */
	    s = verify_area(VERIFY_READ, (void *) arg, sizeof(tinfo));
	    if (s)
		return (s);
	    memcpy_fromfs((void *)&tinfo, (const void *)arg, sizeof(tinfo));
	    if (ap_trace)
		printk ("SET_TEMP: t=%d\n", tinfo.t);
	    if (tinfo.s == CCDTS_SET)
		s = apSetTemperature (minor, tinfo.t);
	    else
		s = apCoolerOff(minor);
	    if (s < 0)
		return (-ENXIO);
	    break;

	case CCD_GET_TEMP: /* get temperature status */
	    s = verify_area(VERIFY_WRITE, (const void *) arg, sizeof(tinfo));
	    if (s)
		return (s);
	    if (ap_ReadRATempStatus(minor, &tinfo) < 0)
		return (-ENXIO);
	    if (ap_trace)
		printk ("GET_TEMP: t=%d\n", tinfo.t);
	    memcpy_tofs((void *)arg, (void *)&tinfo, sizeof(tinfo));
	    break;

	case CCD_GET_SIZE: /* return max image size supported */
	    s = verify_area(VERIFY_WRITE, (const void *) arg, sizeof(cep));
	    if (s)
		return (s);

	    /* reduce size to account for fixed border.
	     * see apSetupExposure().
	     */
	    cep.sw = cdp->cols - (cdp->leftb + cdp->rightb);
	    cep.sh = cdp->rows - (cdp->topb + cdp->botb);
	    cep.bx = MAX_HBIN;
	    cep.by = MAX_VBIN;
	    memcpy_tofs((void *)arg, (void *)&cep, sizeof(cep));
	    if (ap_trace)
		printk ("Max image %dx%d\n", cep.sw, cep.sh);
	    break;

	case CCD_GET_ID: /* return an id string to arg */
	    s = verify_area(VERIFY_WRITE, (const void *) arg, sizeof(id));
	    if (s)
		return (s);
	    if (ap_nfound > 1)
		(void) sprintf (id, "%s %d", ap_id, minor);
	    else
		(void) sprintf (id, "%s", ap_id);
	    memcpy_tofs ((void *)arg, (void *)id, sizeof(id));
	    if (ap_trace)
		printk ("ID: %s\n", id);
	    break;

	case CCD_GET_DUMP: /* reg dump */
	    s = verify_area(VERIFY_WRITE, (const void *) arg, sizeof(cdp->reg));
	    if (s)
		return (s);
	    memcpy_tofs ((void *)arg, (void *)cdp->reg, sizeof(cdp->reg));
	    break;

	case CCD_TRACE: /* set ap_trace from arg. */
	    ap_trace = (int)arg;
	    printk ("%s: tracing is %s\n", ap_name, ap_trace ? "on" : "off");
	    break;

	default: /* huh? */
	    if (ap_trace)
		printk ("ap_ioctl() cmd=%d\n", cmd);
	    return (-EINVAL);
	}

	/* ok */
	return (0);
}

/* return 1 if pixels are ready else sleep_wait() and return 0.
 */
static int
ap_select (struct inode *inode, struct file *file, int sel_type,
select_table *wait)
{
	int minor = MINOR (inode->i_rdev);

	if (ap_trace > 1)
	    printk ("ap_select(): sel_type=%d wait=%d\n", sel_type, !!wait);

        switch (sel_type) {
        case SEL_EX:
        case SEL_OUT:
            return (0); /* never any exceptions or anything to write */
        case SEL_IN:
	    if (ap_state[minor] == AP_EXPDONE) {
		if (ap_trace > 1)
		    printk ("ap_select(): pixels ready\n");
		return (1);
	    }
            break;
        }

        /* pixels not ready -- set timer to check again later and wait */
	startSelectTimer (minor);
	select_wait (&ap_swq[minor], wait);

        return (0);
}

/* the master hook into the support functions for this driver */
static struct file_operations ap_fops = {
    ap_lseek,
    ap_read,
    NULL,	/* write */
    NULL,	/* readdir */
    ap_select,
    ap_ioctl,
    NULL,	/* mmap */
    ap_open,
    ap_release,
    NULL,	/* fsync */
    NULL,	/* fasync */
    NULL,	/* check media change */
    NULL,	/* revalidate */
};

/* called by Linux when we are installed */
int 
init_module (void)
{
	int minor;

	/* scan all possible minor numbers.
	 * N.B. don't commit linux resources until after the loop.
	 */
	for (ap_nfound = minor = 0; minor < MAXCAMS; minor++) {
	    CAMDATA *cdp = &camdata[minor];
	    int base = apg[minor][APP_IOBASE];
	    CCDExpoParams *cep = &ap_cep[minor];

	    /* assume no device until we know otherwise */
	    ap_state[minor] = AP_ABSENT;

	    /* skip if no base address defined */
	    if (!base)
		continue;

	    /* see whether these IO ports are already allocated */
	    if (check_region (base, AP_IO_EXTENT)) {
		printk ("%s[%d]: 0x%x already in use\n", ap_name, minor, base);
		continue;
	    }

	    /* probe device */
	    if (!apProbe(base)) {
		printk ("%s[%d] at 0x%x: not present or not ready\n",
							ap_name, minor, base);
		continue;
	    }

	    /* perform any one-time initialization and more sanity checks */
	    if (apInit (minor, apg[minor]) < 0) {
		printk ("%s[%d]: init failed\n", ap_name, minor);
		continue;
	    }
	    cep->sw = cdp->cols - (cdp->leftb + cdp->rightb);
	    cep->sh = cdp->rows - (cdp->topb + cdp->botb);
	    cep->bx = cdp->hbin;
	    cep->by = cdp->vbin;

	    /* set up pixel bias shift */
	    cdp->pixbias = apg[minor][APP_16BIT] ? 32768 : 0;

	    ap_state[minor] = AP_CLOSED;
	    ap_nfound++;
	}

	if (ap_nfound == 0) {
	    printk ("No good cameras found or bad module parameters.\n");
	    printk ("  Example: apg0=0x290,520,796,2,2,2,2,100,210,100,0,1\n");
	    return (-EINVAL);
	}

	/* install our handler functions -- AP_MJR might be in use */
	if (register_chrdev (AP_MJR, ap_name, &ap_fops) == -EBUSY) {
	    printk ("%s: major %d already in use\n", ap_name, AP_MJR);
	    return (-EIO);
	}

	/* sanity check ap_impact */
	if (ap_impact < 1)
	    ap_impact = 1;

	/* ok, register the io regions as in use with linux */
	for (minor = 0; minor < MAXCAMS; minor++) {
	    int base;

	    if (ap_state[minor] == AP_ABSENT)
		continue;
	    base = apg[minor][APP_IOBASE];
	    printk ("%s[%d] at 0x%x: Version %d.%d ready. Impact level %d\n",
			ap_name, minor,base, AP_MAJVER, AP_MINVER, ap_impact);
	    request_region (base, AP_IO_EXTENT, ap_name);
	}

	/* all set to go */
	return (0);
}

/* called by Linux when module is uninstalled */
void
cleanup_module (void)
{
	int minor;

	for (minor = 0; minor < MAXCAMS; minor++) {
	    int base;

	    if (ap_state[minor] == AP_ABSENT)
		continue;
	    base = apg[minor][APP_IOBASE];
	    stopExpTimer(minor);
	    stopSelectTimer(minor);
	    if (ap_state[minor] == AP_EXPOSING)
		apAbortExposure(minor);
	    release_region (base, AP_IO_EXTENT);
	    ap_state[minor] = AP_ABSENT;
	}

	unregister_chrdev (AP_MJR, ap_name);
	printk ("%s: module removed\n", ap_name);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: main.c,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $"};
