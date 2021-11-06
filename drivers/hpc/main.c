/* Linux driver (module actually) for the Spectra Source Instruments' HPC-1 CCD
 * Camera.
 * (C) Copyright 1997 Elwood Charles Downey. All rights reserved.
 *
 *    2 Dec 97  Begin work
 *    3 Dec 97  First image
 *
 * To build and install:
 *   cc -Wall -O -DHPC_MJR=123 -DMODULE -D__KERNEL__ -c *.c   # compile each .c
 *   ld -r -o hpc.o *.o				# link into one .o
 *   mknod hpc c 123 0	        		# make a special file
 *   insmod hpc.o hpc=addr,impact		# install module (see below)
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
 * When installing with insmod, the camera parameters must be set using hpc0
 *   for the first camera, etc, as below. Also, hpc_impact may be set to
 *   adjust the degree to which reading pixels impacts other system activities.
 *   1 is minimal impact, higher numbers have greater impact but result in
 *   better images. If your images consistently have horizontal streaks in
 *   them, you need a higher number. Go gradually and make it no larger than
 *   necessary.
 *
 * TODO: finish support for multiple devices. All the supporting functions
 *  already accept minor device number. We just need to make the state and
 *  timers arrays here.
 *
 */

#include <linux/version.h>
#include <linux/module.h>

#include "hpc.h"			/* glue */

#define HPC_MAJVER	1		/* major version number */
#define HPC_MINVER	0		/* minor version number */

/* user-defineable options: */

static char hpc_name[] = "hpc";		/* name known to module tools */
static char hpc_id[] = "Specta Source HPC-1"; /* id string */

/* The following arrays hold values which are set when the module is loaded
 * via insmod. The only value is the base address.
 *
 * example install command:
 *
 *   insmod hpc.o hpc0=0x120 hpc_impact=33
 *
 */

/* number of arrays should match MAXCAMS.
 * would make it a 2d array if knew how to init it from insmod :-(
 */
static int hpc0[HPP_N];
static int *hpc[MAXCAMS] = {hpc0};

/* end of user-defineable options */

int hpc_trace;				/* set for printk traces; > for more */
int hpc_impact;				/* pixel reading impact; see hpc.c */
static int hpc_nfound;			/* how many cameras are connected */

#define	HPC_IO_EXTENT	12		/* bytes of I/O space */
#define	HPC_SELTO	(20*HZ/100)	/* select() polling rate, jiffies */

/* driver state */
typedef enum {
    HPC_ABSENT,				/* controller not known to be present */
    HPC_CLOSED,				/* installed, but not now in use */
    HPC_OPEN,				/* open for full use, but idle */
    HPC_EXPOSING,			/* taking an image */
    HPC_EXPDONE				/* image ready to be read */
} HPCState;

/* all of these are indexed by minor device number */
static CCDExpoParams hpc_cep[MAXCAMS];	/* user's exposure details */
static HPCState hpc_state[MAXCAMS];	/* current state of driver */
static struct wait_queue *hpc_wq[MAXCAMS];/* expose-completion wait q */
static struct timer_list hpc_tl[MAXCAMS];/* expose-completion timer list */
static int hpc_timeron[MAXCAMS];		/* set when expose timer is on */
static struct wait_queue *hpc_swq[MAXCAMS];/* used for select() */
static struct timer_list hpc_stl[MAXCAMS];/* used for select() */
static int hpc_stimeron[MAXCAMS];	/* set when select timer is on */

/* handy way to pause the current process for n jiffies.
 * N.B. do _not_ use this with select().
 */
void
hpc_pause (int n)
{
	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + n;
	schedule();
}

/* exposure is complete -- wake up hpc_wq */
static void
expComplete (unsigned long minor)
{
	if (hpc_trace)
	    printk ("expComplete()\n");
	hpc_state[minor] = HPC_EXPDONE;
	hpc_timeron[minor] = 0;
	wake_up_interruptible (&hpc_wq[minor]);
}

/* sleep until the exposure is complete, as indicated by a wakeup on hpc_wq. */
static void
waitForExpTimer(int minor)
{
	if (hpc_trace)
	    printk ("waitForExpTimer()\n");
	interruptible_sleep_on (&hpc_wq[minor]);
}

/* turn off any pending exposure timer, if any. */
static void
stopExpTimer(int minor)
{
	if (hpc_timeron[minor]) {
	    del_timer (&hpc_tl[minor]);
	    hpc_timeron[minor] = 0;
	}
}

/* start a fresh timer for the duration of the exposure */
static void
startExpTimer(int minor)
{
	if (hpc_trace)
	    printk ("startExpTimer()\n");
	stopExpTimer(minor);
	init_timer (&hpc_tl[minor]);
	hpc_tl[minor].expires = jiffies + hpc_cep[minor].duration*HZ/1000 + 1;
	hpc_tl[minor].function = expComplete;	/* will wakeup hpc_wq */
	hpc_tl[minor].data = minor;
	add_timer (&hpc_tl[minor]);
	hpc_timeron[minor] = 1;
}

/* turn off any pending select() timer, if any. */
static void
stopSelectTimer(int minor)
{
	if (hpc_stimeron[minor]) {
	    del_timer (&hpc_stl[minor]);
	    hpc_stimeron[minor] = 0;
	}
}

/* called to start another wait/poll interval on behalf of select() */
static void
startSelectTimer (int minor)
{
	if (hpc_trace > 1)
	    printk ("startSelectTimer()\n");
	stopSelectTimer(minor);
	init_timer (&hpc_stl[minor]);
	hpc_stl[minor].expires = jiffies + HPC_SELTO;
	hpc_stl[minor].function = (void(*)(unsigned long))wake_up_interruptible;
	hpc_stl[minor].data = (unsigned long) &hpc_swq[minor];
	add_timer (&hpc_stl[minor]);
	hpc_stimeron[minor] = 1;
}

/* open the device and return 0, else a errno.
 */
static int
hpc_open (struct inode *inode, struct file *file)
{
	int minor = MINOR (inode->i_rdev);
	int flags = file->f_flags;

	if (hpc_trace)
	    printk ("hpc_open(%d): hpc_state=%d\n", minor, hpc_state[minor]);
	if (minor < 0 || minor >= MAXCAMS)
	    return (-ENODEV);
	if (hpc_state[minor] == HPC_ABSENT)
	    return (-ENODEV);
	if (hpc_state[minor] != HPC_CLOSED)
	    return (-EBUSY);

	/* there's no write but RDWR captures the spirit of ioctl. */
	if ((flags & O_ACCMODE) != O_RDWR)
	    return (-EACCES);

	if (hpcReset(minor) < 0)
	    return (-EACCES);

	hpc_state[minor] = HPC_OPEN;
	MOD_INC_USE_COUNT;
	return (0);
}    

/* called when driver is closed. clean up and decrement module use count */
static void
hpc_release (struct inode *inode, struct file *file)
{
	int minor = MINOR (inode->i_rdev);

	if (hpc_trace)
	    printk ("hpc_release()\n");
	stopExpTimer(minor);
	stopSelectTimer(minor);
	if (hpc_state[minor] == HPC_EXPOSING)
	    hpcAbortExposure(minor);
	hpc_state[minor] = HPC_CLOSED;
	MOD_DEC_USE_COUNT;
}    

/* fake system call for lseek -- just so cat works */
static int
hpc_lseek (struct inode *inode, struct file *file, off_t offset, int orig)
{
	if (hpc_trace)
	    printk ("hpc_lseek()\n");
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
hpc_read (struct inode *inode, struct file *file, char *buf, int count)
{
	int minor = MINOR (inode->i_rdev);
	CCDExpoParams *cep = &hpc_cep[minor];
	int nbytes = cep->sw/cep->bx * cep->sh/cep->by * 2;
	int s;

	if (hpc_trace)
	    printk ("hpc_read() hpc_state=%d\n", hpc_state[minor]);

	/* sanity check buf if we are really going to use it */
	if (hpc_state[minor] == HPC_EXPDONE || !(file->f_flags & O_NONBLOCK)) {
	    if (((unsigned)buf) & 1)
		return (-EFAULT);
	    s = verify_area(VERIFY_WRITE, (const void *) buf, nbytes);
	    if (s)
		return (s);
	}

	if (hpc_state[minor] == HPC_OPEN) {
	    if (hpcStartExposure(minor) < 0)
		return (-ENXIO);
	    startExpTimer(minor);
	    hpc_state[minor] = HPC_EXPOSING;
            if (file->f_flags & O_NONBLOCK)
		return (0);
	}

	if (hpc_state[minor] == HPC_EXPOSING) {
            if (file->f_flags & O_NONBLOCK)
		return (-EAGAIN);
	    waitForExpTimer(minor);
	    if (hpc_state[minor] == HPC_EXPOSING) {
		/* still exposing means sleep was interrupted -- abort */
		stopExpTimer(minor);
		hpcAbortExposure (minor);
		hpc_state[minor] = HPC_OPEN;
		return (-EINTR);
	    }
	    hpc_state[minor] = HPC_EXPDONE;
	}

	if (hpc_state[minor] != HPC_EXPDONE) {
	    printk ("%s[%d]: hpc_read but state is %d\n", hpc_name, minor,
							    hpc_state[minor]);
	    hpc_state[minor] = HPC_OPEN;
	    return (-EIO);
	}

	s = hpcDigitizeCCD (minor, (unsigned short *)buf);
	hpc_state[minor] = HPC_OPEN;
	return (s < 0 ? -EPERM : nbytes);
}

/* system call interface for ioctl.
 * we support setting exposure params, turning the cooler on and off, reading
 * the camera temperature.
 */
static int
hpc_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
unsigned long arg)
{
	int minor = MINOR (inode->i_rdev);
	char id[sizeof(hpc_id) + 10];
	CCDExpoParams cep;
	CCDTempInfo tinfo;
	int s;

	switch (cmd) {
	case CCD_SET_EXPO: /* save caller's CCDExpoParams, at arg */
	    s = verify_area(VERIFY_READ, (void *) arg, sizeof(cep));
	    if (s)
		return (s);
	    memcpy_fromfs((void *)&cep, (const void *)arg, sizeof(cep));

	    if (hpc_trace)
		printk ("ExpoParams: %d/%d %d/%d/%d/%d %d ms\n", cep.bx, cep.by,
				cep.sx, cep.sy, cep.sw, cep.sh, cep.duration);

	    if (cep.duration < 0 || hpcSetupExposure (minor, &cep) < 0)
		return (-ENXIO);

	    hpc_cep[minor] = cep;
	    break;

	case CCD_SET_TEMP: /* set temperature */
	    s = verify_area(VERIFY_READ, (void *) arg, sizeof(tinfo));
	    if (s)
		return (s);
	    memcpy_fromfs((void *)&tinfo, (const void *)arg, sizeof(tinfo));
	    if (hpc_trace)
		printk ("SET_TEMP: t=%d\n", tinfo.t);
	    if (tinfo.s == CCDTS_SET)
		s = hpcSetTemperature (minor, tinfo.t);
	    else
		s = hpcCoolerOff(minor);
	    if (s < 0)
		return (-ENXIO);
	    break;

	case CCD_GET_TEMP: /* get temperature status */
	    s = verify_area(VERIFY_WRITE, (const void *) arg, sizeof(tinfo));
	    if (s)
		return (s);
	    if (hpcReadTempStatus(minor, &tinfo) < 0)
		return (-ENXIO);
	    if (hpc_trace)
		printk ("GET_TEMP: t=%d\n", tinfo.t);
	    memcpy_tofs((void *)arg, (void *)&tinfo, sizeof(tinfo));
	    break;

	case CCD_GET_SIZE: /* return max image size supported */
	    s = verify_area(VERIFY_WRITE, (const void *) arg, sizeof(cep));
	    if (s)
		return (s);

	    /* reduce size to account for fixed border.
	     * see hpcSetupExposure().
	     */
	    cep.sw = X_EXTENT;
	    cep.sh = Y_EXTENT;
	    cep.bx = MAX_HBIN;
	    cep.by = MAX_VBIN;
	    memcpy_tofs((void *)arg, (void *)&cep, sizeof(cep));
	    if (hpc_trace)
		printk ("Max image %dx%d\n", cep.sw, cep.sh);
	    break;

	case CCD_GET_ID: /* return an id string to arg */
	    s = verify_area(VERIFY_WRITE, (const void *) arg, sizeof(id));
	    if (s)
		return (s);
	    if (hpc_nfound > 1)
		(void) sprintf (id, "%s %d", hpc_id, minor);
	    else
		(void) sprintf (id, "%s", hpc_id);
	    memcpy_tofs ((void *)arg, (void *)id, sizeof(id));
	    if (hpc_trace)
		printk ("ID: %s\n", id);
	    break;

	case CCD_GET_DUMP: /* reg dump */
	    return (-ENOENT); 	/* not implemented */
	    break;

	case CCD_TRACE: /* set hpc_trace from arg. */
	    hpc_trace = (int)arg;
	    printk ("%s: tracing is %s\n", hpc_name, hpc_trace ? "on" : "off");
	    break;

	default: /* huh? */
	    if (hpc_trace)
		printk ("hpc_ioctl() cmd=%d\n", cmd);
	    return (-EINVAL);
	}

	/* ok */
	return (0);
}

/* return 1 if pixels are ready else sleep_wait() and return 0.
 */
static int
hpc_select (struct inode *inode, struct file *file, int sel_type,
select_table *wait)
{
	int minor = MINOR (inode->i_rdev);

	if (hpc_trace > 1)
	    printk ("hpc_select(): sel_type=%d wait=%d\n", sel_type, !!wait);

        switch (sel_type) {
        case SEL_EX:
        case SEL_OUT:
            return (0); /* never any exceptions or anything to write */
        case SEL_IN:
	    if (hpc_state[minor] == HPC_EXPDONE) {
		if (hpc_trace > 1)
		    printk ("hpc_select(): pixels ready\n");
		return (1);
	    }
            break;
        }

        /* pixels not ready -- set timer to check again later and wait */
	startSelectTimer (minor);
	select_wait (&hpc_swq[minor], wait);

        return (0);
}

/* the master hook into the support functions for this driver */
static struct file_operations hpc_fops = {
    hpc_lseek,
    hpc_read,
    NULL,	/* write */
    NULL,	/* readdir */
    hpc_select,
    hpc_ioctl,
    NULL,	/* mmap */
    hpc_open,
    hpc_release,
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
	for (hpc_nfound = minor = 0; minor < MAXCAMS; minor++) {
	    int base = hpc[minor][HPP_IOBASE];
	    CCDExpoParams *cep = &hpc_cep[minor];

	    /* assume no device until we know otherwise */
	    hpc_state[minor] = HPC_ABSENT;

	    /* skip if no base address defined */
	    if (!base)
		continue;

	    /* see whether these IO ports are already allocated */
	    if (check_region (base, HPC_IO_EXTENT)) {
		printk ("%s[%d]: 0x%x already in use\n", hpc_name, minor, base);
		continue;
	    }

	    /* probe device */
	    if (!hpcProbe(base)) {
		printk ("%s[%d] at 0x%x: not present or not ready\n",
							hpc_name, minor, base);
		continue;
	    }

	    /* perform any one-time initialization and more sanity checks */
	    if (hpcInit (minor, hpc[minor]) < 0) {
		printk ("%s[%d]: init failed\n", hpc_name, minor);
		continue;
	    }
	    cep->sw = X_EXTENT;
	    cep->sh = Y_EXTENT;
	    cep->bx = 1;
	    cep->by = 1;

	    hpc_state[minor] = HPC_CLOSED;
	    hpc_nfound++;
	}

	if (hpc_nfound == 0) {
	    printk ("No good cameras found or bad module parameters.\n");
	    printk ("  Example: hpc0=0x120 hpc_impact=3\n");
	    return (-EINVAL);
	}

	/* install our handler functions -- HPC_MJR might be in use */
	if (register_chrdev (HPC_MJR, hpc_name, &hpc_fops) == -EBUSY) {
	    printk ("%s: major %d already in use\n", hpc_name, HPC_MJR);
	    return (-EIO);
	}

	/* sanity check hpc_impact */
	if (hpc_impact < 1)
	    hpc_impact = 1;

	/* ok, register the io regions as in use with linux */
	for (minor = 0; minor < MAXCAMS; minor++) {
	    int base;

	    if (hpc_state[minor] == HPC_ABSENT)
		continue;
	    base = hpc[minor][HPP_IOBASE];
	    printk ("%s[%d] at 0x%x: Version %d.%d ready. Impact level %d\n",
		    hpc_name, minor,base, HPC_MAJVER, HPC_MINVER, hpc_impact);
	    request_region (base, HPC_IO_EXTENT, hpc_name);
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

	    if (hpc_state[minor] == HPC_ABSENT)
		continue;
	    base = hpc[minor][HPP_IOBASE];
	    stopExpTimer(minor);
	    stopSelectTimer(minor);
	    if (hpc_state[minor] == HPC_EXPOSING)
		hpcAbortExposure(minor);
	    release_region (base, HPC_IO_EXTENT);
	    hpc_state[minor] = HPC_ABSENT;
	}

	unregister_chrdev (HPC_MJR, hpc_name);
	printk ("%s: module removed\n", hpc_name);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: main.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
