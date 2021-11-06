/* Linux module for the Oregon Micro Systems PC39 Intelligent Motor Controller.
 *   compile: cc -O -DPC39_MJR=121 -DMODULE -D__KERNEL__ -c pc39.c
 *   install: insmod pc39.0 [pc39_io_base=0x???]
 *   mknod:   mknod /dev/pc39 c 121 0
 * we support open/close/read/write/select/ioctl.
 * we allow any number of simultaneous opens, but PC39_OUTIN ioctl is atomic.
 * we do not use interrupts. set pc39_io_base to the desired base address.
 *   
 * Elwood Downey
 * 19 Jul 96 Begin
 * 22 Jul 96 Add ioctls and probing.
 * 23 Jul 96 Check/Register/Release io ports. select() digs a little.
 * 13 Nov 96 Get select() correct.
 * 21 May 97 Add fast spin loops for read and write -- 10x faster now.
 * 22 May 97 Clear CMD_S and OVRT errors whenever they are encountered.
 *           Precede each command with ' ' to be sure it's freshly delimited.
 *           Add busy wait timeout.
 * 25 Sep 97 Add notion of version (this is 2). More precise CRLF handling.
 * 14 May 98 change to tight loops and schedule(). add simple /proc/pc39.
 * 24 May 98 stop trying to prefix /proc/pc39 with < and >.
 *  4 Feb 00 update for 2.2.* kernels
 * 21 Feb 00 need to wrap insmod params
 * 25 Feb 00 fix bugs introduced in pre-2.2 code :(
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <asm/io.h>

// can't include stdlib for compatibility reasons...
void exit(int status);

#if (LINUX_VERSION_CODE < 0x020200)
#include <asm/segment.h>
#define	CHKIO(RW,a,n)	if(verify_area(RW,(const void *)a, n)) return (-EFAULT)
#define	GETU(l,a)	l = get_user(a)
#define	CPYF(t,f,n)	memcpy_fromfs (t, f, n)
#define	CPYT(t,f,n)	memcpy_tofs (t, f, n)
#else
#include <asm/uaccess.h>
#include <linux/poll.h>
#define	CHKIO(RW,a,n)	if(!access_ok(RW,(const void *)a, n)) return (-EFAULT)
#define	GETU(l,a)	get_user(l,a)
#define	CPYF(t,f,n)	__copy_from_user (t, f, n)
#define	CPYT(t,f,n)	__copy_to_user (t, f, n)
#endif

#include "pc39.h"

#define	PC39_MAJVER	2		/* major version number */
#define	PC39_MINVER	2		/* minor version number */

#if (LINUX_VERSION_CODE >= 0x020200)
MODULE_PARM(pc39_io_base, "i");
#endif

int pc39_io_base = 0x360;		/* base address -- insmod can override*/
#define	PC39_IO_EXTENT	4		/* bytes of I/O space */

#define	PC39_RSTO	(2*HZ)		/* wait for RS to work, jiffies */
#define	PC39_BTO	(4*HZ)		/* timeout to wait our turn, jiffies */
#define	PC39_WTO	(HZ)		/* timeout to write one char, jiffies */
#define	PC39_RTO	(HZ)		/* timeout to read one char, jiffies */
#define	PC39_SELTO	(100*HZ/1000)	/* select() polling delay, jiffies */

static char pc39_busy;			/* flag used for atomic PC39_OUTIN */
static char pc39_present;		/* flag set when we know pc39 exists */
static struct wait_queue *pc39_wq;

#if (LINUX_VERSION_CODE < 0x020200)
static char pc39_timeron;		/* flag set when select timer is on */
static struct timer_list pc39_tl;	/* used for select() */
#endif

#define	PC39_PBS	512		/* /proc/pc39 buffer size */
static char pc39_pb[PC39_PBS];		/* /proc/pc39 buffer */
static int pc39_pbh;			/* index for next char onto pb */
#define PC39_PB_ENQ(c)	(pc39_pb[(pc39_pbh++)%PC39_PBS] = (c)) /* enqueue to */

/* macros to access the four pc39 i/o regs.
 * use these directly with INB and OUTB.
 */
#define	PC39_REG_DATA		(pc39_io_base + 0)
#define	PC39_REG_DONE		(pc39_io_base + 1)
#define	PC39_REG_CTRL		(pc39_io_base + 2)
#define	PC39_REG_STAT		(pc39_io_base + 3)
#define	INB(p)			(inb_p(p) & 0xff)
#define	OUTB(b,p)		(outb_p(b,p))

#define	IBF()			(INB(PC39_REG_STAT) & PC39_STAT_IBF_S)
#define	TBE()			(INB(PC39_REG_STAT) & PC39_STAT_TBE_S)

#define	CR			'\r'
#define	LF			'\n'
#define	CTRLD			4

#define	PC39_CTRLX		0x18	/* ^X clears following errors */
#define	PC39_CTRLXERRS		(PC39_STAT_CMD_S|PC39_STAT_OVRT)

/* read a byte from PC39_REG_DATA and shadow in pc39_pb[] */
static int
pc39_indata ()
{
	char c = INB(PC39_REG_DATA);
	PC39_PB_ENQ(c);
	return (c);
}

/* write a byte to PC39_REG_DATA and shadow in pc39_pb[] */
static void
pc39_outdata (char c)
{
	OUTB(c, PC39_REG_DATA);
	PC39_PB_ENQ(c);
}

/* pause for n jiffies.
 * N.B. do _not_ use this with select().
 */
static void
pc39_pause (int n)
{
#if (LINUX_VERSION_CODE >= 0x020200)
	interruptible_sleep_on_timeout(&pc39_wq, (long)n);
#else
	current->timeout = jiffies + n;
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	current->timeout = 0;
#endif /* (LINUX_VERSION_CODE >= 0x020200) */
}

/* read and return one character.
 * if nowait we don't wait around at all.
 * return the char or -1 if none.
 */
static int
pc39_rb(int nowait)
{
	unsigned long jto;

	/* if nowait, we only check once */
	if (nowait)
	    return (IBF() ? pc39_indata() : -1);

	jto = jiffies + PC39_RTO;
	while (jiffies < jto) {
	    if (IBF())
		return (pc39_indata());
	    schedule();	/* hog, albeit politely */
	}

	/* clear the control reg and repeat */
	OUTB(0, PC39_REG_CTRL);

	jto = jiffies + PC39_RTO;
	while (jiffies < jto) {
	    if (IBF())
		return (pc39_indata());
	    schedule();	/* hog, albeit politely */
	}

	/* sorry */
	printk ("pc39: read-byte timeout\n");
	return (-1);
}

/* write one byte. return 0 if ok, else -1 */
static int
pc39_wb(char b)
{
	unsigned long jto;

	jto = jiffies + PC39_WTO;
	while (jiffies < jto) {
	    if (TBE()) {
		pc39_outdata(b);
		return (0);
	    }
	    schedule();	/* hog, albeit politely */
	}

	/* clear the control reg and repeat */
	OUTB(0, PC39_REG_CTRL);

	jto = jiffies + PC39_WTO;
	while (jiffies < jto) {
	    if (TBE()) {
		pc39_outdata(b);
		return (0);
	    }
	    schedule();	/* hog, albeit politely */
	}

	/* nope */
	return (-1);
}

/* read next message into user buf[count], and add '\0'.
 * discard the leading and trailing CR/LF delimiters.
 * N.B. a whole message is read from the device, even if it doesn't fit in buf.
 * return total chars returned, including '\0', else -errno.
 */
static int
pc39_rm (char *buf, int count)
{
	int c;
	int n;

	/* insure buf has room for at least the trailing '\0' we always add */
	if (count < 1)
	    return (-ENOENT);

	/* check for valid user buffer */
	CHKIO(VERIFY_WRITE, buf, count);

	/* get the first non-CRLF into c. */
	do {
	    if ((c = pc39_rb(0)) < 0)
		return (-ETIMEDOUT);
	} while (c == CR || c == LF);

	/* read to the next LF, passing back as much as fits in buf */
	n = 0;
	do {
	    if (n < count-1) {
		put_user (c, buf);
		buf++;
		n++;
	    }
	} while ((c = pc39_rb(0)) >= 0 && c != LF);

	/* eat another CR for sure. some messages will have 2 but select() will
	 * eat it -- we don't want to wait for it here.
	 */
	if (pc39_rb (0) < 0)
	    return (-EIO);

	/* add '\0' */
	put_user ('\0', buf);
	n++;

	/* return number of chars put in buf[], including trailing '\0' */
	return (n);
}

/* write user buf[count] to the device.
 * return total chars written, or -errno if trouble.
 */
static int
pc39_wm (const char *buf, int count)
{
	int lastc = 10;
	int n;

	/* check for valid user buffer */
	CHKIO (VERIFY_READ, buf, count);

	/* write count chars from buf, watching for possible timeout */
	for (n = 0; n < count; n++) {
	    GETU (lastc, buf);
	    if (pc39_wb (lastc) < 0) {
		printk ("wr: failed after %d\n", n);
		return (-EIO);
	    }
	    buf++;
	}

	/* add a 10 to be sure last command gets delimited */
	if (lastc != 10 && pc39_wb (10) < 0)
	    return (-EIO);

	/* return count of chars actually written */
        return (n);
}

/* wait for !pc39_busy. return 0 if ok, -1 if timeout. */
static int
pc39_busywait()
{
	unsigned long jto;

	jto = jiffies + PC39_BTO;
	while (jiffies < jto) {
	    if (!pc39_busy)
		return (0);
	    pc39_pause(10*HZ/1000);
	}
	return (-1);
}

/* fake system call for lseek -- just so cat works */
#if (LINUX_VERSION_CODE >= 0x020200)
static loff_t
pc39_lseek (struct file *file, loff_t offset, int orig)
#else
static int
pc39_lseek (struct inode *inode, struct file *file, off_t offset, int orig)
#endif
{
	return (file->f_pos = 0);
}

/* system call interface to read next message into buf[count].
 * discard the leading and trailing <LF><CR> delimiters.
 * N.B. a whole message is read from the device, even if it doesn't fit in buf.
 * N.B. this implies one may not read char-at-a-time.
 * N.B. we wait for PC39_OUTIN ioctl() to complete, if necessary.
 * N.B. we participate in pc39_busy.
 * return total chars returned, else -errno.
 */
#if (LINUX_VERSION_CODE >= 0x020200)
static ssize_t
pc39_read (struct file *file, char *buf, size_t count, loff_t *offset)
#else
static int
pc39_read (struct inode *node, struct file *file, char *buf, int count)
#endif
{
	int s;

	/* wait our (random) turn */
	if (pc39_busywait() < 0)
	    return (-EBUSY);
        pc39_busy = 1;

	/* read a message */
	s = pc39_rm (buf, count);

	/* give up our turn */
        pc39_busy = 0;

	/* return count or error code */
	return (s);
}

/* system call interface to write buf[count] to the device.
 * return total chars written, else -errno.
 * N.B. we wait for PC39_OUTIN ioctl() to complete, if necessary.
 * N.B. we participate in pc39_busy.
 * N.B. for message locking to work, write whole commands, not char-at-a-time.
 */
#if (LINUX_VERSION_CODE >= 0x020200)
static ssize_t
pc39_write (struct file *file, const char *buf, size_t count, loff_t *offset)
#else
static int
pc39_write (struct inode *node, struct file *file, const char *buf, int count)
#endif
{
	int s;

	/* wait our (random) turn */
	if (pc39_busywait() < 0)
	    return (-EBUSY);
        pc39_busy = 1;

	/* write the message */
	s = pc39_wm (buf, count);

	/* give up our turn */
        pc39_busy = 0;

	/* return count or error code */
	return (s);
}

/* system call interface for ioctl.
 * return whatever, or -errno if trouble.
 */
static int
pc39_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
unsigned long arg)
{
	PC39_OutIn oi;
	PC39_Regs regs;
	int r;

	switch (cmd) {
	case PC39_GETREGS:
	    /* read the done/ctrl/stat regs.
	     * N.B. some bits are cleared when read
	     */

	    CHKIO (VERIFY_WRITE, arg, sizeof(PC39_Regs));
	    regs.done = INB(PC39_REG_DONE);
	    regs.ctrl = INB(PC39_REG_CTRL);
	    regs.stat = INB(PC39_REG_STAT);
	    CPYT ((PC39_Regs *)arg, &regs, sizeof(regs));

	    return (0);

	case PC39_OUTIN:
	    /* atomic write/read, with preflush */

	    /* wait our (random) turn */
	    if (pc39_busywait() < 0)
		return (-EBUSY);

	    /* get user buffer */
	    CHKIO (VERIFY_READ, arg, sizeof(PC39_OutIn));
	    pc39_busy = 1;
	    CPYF (&oi, (PC39_OutIn *)arg, sizeof(oi));

	    /* flush any old responses - we want strictly this pair */
	    while (pc39_rb(1) >= 0)
		continue;

	    /* send out[] message and read response in in[] */
	    r = pc39_wm (oi.out, oi.nout);
	    if (r >= 0)
		r = pc39_rm (oi.in, oi.nin);
	    pc39_busy = 0;

	    /* return status or chars in in[] */
	    return (r);

	default:
	    break;
	}

	return (-EINVAL);
}

#if (LINUX_VERSION_CODE < 0x020200)
static void
pc39_stopSelectTimer()
{
	if (pc39_timeron) {
	    del_timer (&pc39_tl);
	    pc39_timeron = 0;
	}
}

/* called to start another wait/poll interval on behalf of select() */
static void
pc39_startSelectTimer ()
{
	pc39_stopSelectTimer();
	init_timer (&pc39_tl);
	pc39_tl.expires = jiffies + PC39_SELTO;
	pc39_tl.function = (void(*)(unsigned long))wake_up_interruptible;
	pc39_tl.data = (unsigned long) &pc39_wq;
	add_timer (&pc39_tl);

	pc39_timeron = 1;
}

/* return 1 if device is ready with a new message now else sleep_wait() and
 * return 0.
 * N.B. we participate in pc39_busy.
 */
static int
pc39_select (struct inode *inode, struct file *file, int sel_type,
select_table *wait)
{
        switch (sel_type) {
        case SEL_EX:
            return (0); /* never any exceptions */

        case SEL_IN:
            if (!pc39_busy && IBF()) {
		/* all messages start with LF. anything else is just junk */
		if (pc39_rb(1) == LF) {
		    pc39_stopSelectTimer();
		    return (1);
		}
	    }
            break;

        case SEL_OUT:
            if (!pc39_busy && TBE()) {
		pc39_stopSelectTimer();
                return (1);
	    }
            break;
        }

        /* nothing ready -- set timer to check again later and wait */
	pc39_startSelectTimer ();
	select_wait (&pc39_wq, wait);

        return (0);
}
#else	/* (LINUX_VERSION_CODE >= 0x020200) */
static unsigned int
pc39_select (struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait (file, &pc39_wq, wait);
	if (!pc39_busy && IBF() && pc39_rb(1) == LF)
	    mask |= POLLIN | POLLRDNORM;
	if (!pc39_busy && TBE())
	    mask |= POLLOUT | POLLWRNORM;
	return (mask);
}

#endif /* (LINUX_VERSION_CODE < 0x020200) */

/* if device is present, just increment module installation count; else return
 * ENODEV.
 */
static int
pc39_open (struct inode *inode, struct file *file)
{
	if (!pc39_present)
	    return (-ENODEV);

	MOD_INC_USE_COUNT;
	return (0);
}    

/* just decrement module installation count */
#if (LINUX_VERSION_CODE >= 0x020200)
static int
pc39_release (struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;
	return(0);
}    
#else
static void
pc39_release (struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;
}    
#endif

/* the master hook into the support functions for this driver */
#if (LINUX_VERSION_CODE >= 0x020200)
static struct file_operations pc39_fops = {
    llseek:	pc39_lseek,
    read:	pc39_read,
    write:	pc39_write,
    poll:	pc39_select,
    ioctl:	pc39_ioctl,
    open:	pc39_open,
    release:	pc39_release,
};
#else
static struct file_operations pc39_fops = {
    pc39_lseek,
    pc39_read,
    pc39_write,
    NULL,	/* readdir */
    pc39_select,
    pc39_ioctl,
    NULL,	/* mmap */
    pc39_open,
    pc39_release,
    NULL,	/* fsync */
    NULL,	/* fasync */
    NULL,	/* check media change */
    NULL,	/* revalidate */
};
#endif

/* return 1 if TBE is on and INIT is off, else fuss and return 0 */
static int
pc39_ready()
{
        int i = INB(PC39_REG_STAT);

        if (i & PC39_STAT_INIT) {
            printk ("pc39: INIT is stuck on in status reg: 0x%x\n", i);
            return (0);
        }
        if (!(i & PC39_STAT_TBE_S)) {
            printk ("pc39: TBE is stuck off in status reg: 0x%x\n", i);
            return (0);
        }

        /* ok: TBE and !INIT */
        return (1);
}

/* return non-0 if the pc39 appears to be installed and ready to go, else 0.
 * we try to reset the device if possible before giving up.
 */
static int
pc39_probe()
{
	if (pc39_ready())
	    return (1);

	/* try clearing errors, and resetting the whole board */
	printk ("pc39: Trying to reset\n");
	OUTB(PC39_CTRLX, PC39_REG_DATA);
	(void) pc39_wb('R');
	(void) pc39_wb('S');

	pc39_pause (PC39_RSTO);
	return (pc39_ready());
}

/* make a simple /proc/pc39 entry which provides the last several commands
 * and responses to the board.
 * See Rubin page 75 for how to do /proc entries.
 */
static int
pc39_read_proc (char *buf, char **stat, off_t offset, int len, int unused)
{
	char last_c;
	int i, n;

	/* all-or-nothing */
	if (len < PC39_PBS+10)	/* just spooked */
	    return (0);

	/* mop up many potential overflows while we are here */
	pc39_pbh %= PC39_PBS;

	/* copy back the whole buffer, except any 0s.
	 * replace with 0s to mark as read.
	 * map multiple CR/LF's to just one LF.
	 */
	n = 0;
	last_c = '\0';
	for (i = 0; i < PC39_PBS; i++) {
	    int idx = (pc39_pbh + i)%PC39_PBS;
	    char c = pc39_pb[idx];

	    if (c) {
		pc39_pb[idx] = '\0';
		if (c == CR)
		    c = LF;
		if (c != LF || last_c != LF)
		    buf[n++] = c;
	    }
	    last_c = c;
	}

	/* one last NL if necessary */
	if (n > 0 && buf[n-1] != LF)
	    buf[n++] = LF;

	return (n);
}

static struct proc_dir_entry
pc39_proc_entry = {
    0,			/* low_ino: the inode -- dynamic */
    4, "pc39",		/* len of name and name */
    S_IFREG|S_IRUGO,	/* mode */
    1, 0, 0,		/* nlinks, owner, group */
    0,			/* size -- unused */
    NULL, 		/* operations -- use default */
    &pc39_read_proc,	/* function used to read data */
    /* nothing more */
};

int 
init_module (void)
{
	/* see that our IO ports are not already allocated */
	if (check_region (pc39_io_base, PC39_IO_EXTENT)) {
	    printk ("pc39: IO port 0x%x already in use\n", pc39_io_base);
	    return (-EIO);
	}

	/* probe device */
	if (!pc39_probe()) {
	    printk ("pc39: 0x%x: not present or not ready\n", pc39_io_base);
	    return (-ENODEV);
	}

	/* install our handler functions and register the io ports */
	if (register_chrdev (PC39_MJR, "pc39", &pc39_fops) == -EBUSY) {
	    printk ("pc39: major %d is already in use\n", PC39_MJR);
	    return (-EIO);
	}
	request_region (pc39_io_base, PC39_IO_EXTENT, "pc39");
	pc39_present = 1;

	/* no dma or interrupts */
	OUTB(0, PC39_REG_CTRL);

	/* add /proc/pc39 */
#if (LINUX_VERSION_CODE >= 0x020200)
        proc_register (&proc_root, &pc39_proc_entry);
#else
	proc_register_dynamic (&proc_root, &pc39_proc_entry);
#endif
	
	/* ok */
	printk ("pc39 at 0x%x: Version %d.%d installed and ready\n",
					pc39_io_base, PC39_MAJVER, PC39_MINVER);

	return (0);
}

void
cleanup_module (void)
{
	unregister_chrdev (PC39_MJR, "pc39");
	release_region (pc39_io_base, PC39_IO_EXTENT);
	proc_unregister (&proc_root, pc39_proc_entry.low_ino);
	printk ("pc39: module removed\n");
	pc39_present = 0;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc39.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
