/* driver file for the Oregon Micro Systems, Inc, PC34 intelligent motor cntlr.
 * page numbers refer to the "User's Manual", as revised March 15, 1990.
 * we support open/close/read/write/poll/ioctl.
 * there is only one ioctl: return the current set of done/stat/ctrl.
 * we only support non-blocking reads; writes always block until completed.
 * we do not support the DON interrupt.
 *
 * v1.0	 9/24/93 first reasonable version.
 * v0.1  9/23/93 start work; Elwood C. Downey
 */

#include <sys/types.h>
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

#include <sys/pc34.h>

/* stuff from space.c */
extern int pc34_io_base;
extern int pc34_intpri;

/* we are obliged to provide this; see devflag(D1D) */
int pc34devflag = 0;

/* opaque structure to support poll(2) system call */
static struct pollhead pc34_ph;

/* current state */
#define	PC34_ABSENT	0
#define	PC34_PRESENT	1
#define	PC34_OPEN	2
static int pc34_state = PC34_ABSENT;

/* macros to access the four pc34 i/o regs.
 * use these directly with inb and outb.
 */
#define	PC34_REG_DATA		(pc34_io_base + 0)
#define	PC34_REG_DONE		(pc34_io_base + 1)
#define	PC34_REG_CTRL		(pc34_io_base + 2)
#define	PC34_REG_STAT		(pc34_io_base + 3)

/* processor priority for privileged sections. */
#define	PC34_SPL	pc34_intpri

/* sleep priority */
#define	PC34_SLPPRI	(PZERO+1)	/* signalable if > PZERO */

/* circular receive and transmit q buffers.
 * put next char at XXp ("put"), get next char from XXg ("get").
 * when getting q is empty when put/get indices match.
 * when putting q if full when put/get indices match; discard oldest.
 */
#define	RBUFSZ		64	/* longest rcv possible before loss of data */
static char rbuf[RBUFSZ];	/* receive buffer */
int rbufp, rbufg;		/* index of where to put-on and get-from next */
#define	TBUFSZ		64	/* xmit buf size; does *NOT* imply a max xfer */
static char tbuf[RBUFSZ];	/* transmit buffer */
int tbufp, tbufg;		/* index of where to put-on and get-from next */

/* all the various poll(2) states we claim to be "ready" for.
 */
#define	POLLS	(POLLRDNORM|POLLIN)

/* shadow of ost regs */
static PC34_Regs pc34_regs;

/* see whether the PC34 appears to be here by looking for it to announce
 * that it can accept an input character.
 */
void
pc34init()
{
	int n = 1000000;

	cmn_err (CE_CONT, "PC34: addr=0x%x, intr=%d: ", pc34_io_base,
								pc34_intpri);

	while (--n) {
	    int stat = inb (PC34_REG_STAT);
	    if ((stat & (PC34_STAT_TBE_S|PC34_STAT_INIT)) == PC34_STAT_TBE_S)
		break;
	    if ((n % 200000) == 0)
		cmn_err (CE_CONT, ".");
	}

	if (n > 0) {
	    cmn_err (CE_CONT, "ready.\n");
	    pc34_state = PC34_PRESENT;
	} else
	    cmn_err (CE_CONT, "not found.\n");
}

int
pc34open (dev_t *devp, int oflag, int otyp, cred_t *crp)
{
	int mask;

	switch (pc34_state) {
	case PC34_PRESENT:	break;	/* normal case */
	case PC34_OPEN:		return (EBUSY);
	default: 		return (ENODEV);
	}

	pc34_state = PC34_OPEN;

	/* reset the queues */
	rbufp = rbufg = 0;
	tbufp = tbufg = 0;

	/* enable receive interrupts.
	 * transmit get enabled with each write.
	 */
#ifdef USE_DONE
	mask = PC34_CTRL_IRQ_E|PC34_CTRL_IBF_E|PC34_CTRL_DON_E;
#else
	mask = PC34_CTRL_IRQ_E|PC34_CTRL_IBF_E;
#endif
	outb (PC34_REG_CTRL, mask);

	/* init the shadow regs */
	pc34_regs.stat = inb (PC34_REG_STAT);
	pc34_regs.ctrl = inb (PC34_REG_CTRL);
	pc34_regs.done = inb (PC34_REG_DONE);

	return (0);
}

pc34close()
{
	/* turn all interrupts off */
	outb (PC34_REG_CTRL, 0);

	pc34_state = PC34_PRESENT;

	return (0);
}

/* read data from the PC34, if any.
 * we block until at least one byte is available unless FNDELAY or FNONBLOCK.
 */
pc34read (dev_t dev, uio_t *uiop, cred_t *crp)
{
	int oldpri;

	oldpri = splx (PC34_SLPPRI);

	/* if nothing to read, check for non-blocking reads (old and POSIX) */
	if (rbufg == rbufp) {
	    if (uiop->uio_fmode & FNDELAY) {
		(void) splx (oldpri);
		return (0);
	    }
	    if (uiop->uio_fmode & FNONBLOCK) {
		(void) splx (oldpri);
		return (EAGAIN);
	    }
	}

	/* sleep while there is nothing in the receive q or until a signal */
	while (rbufg == rbufp)
	    if (sleep (rbuf, PC34_SLPPRI|PCATCH)) {
		(void) splx (oldpri);
		return (EINTR);
	    }

	/* copy the receive q up to the user */
	while (rbufg != rbufp) {
	    /* pass one char up to the user.
	     * ureadc "returns 0 on success or an error number on failure".
	     * what does it return if the user's buffer just has no more room?
	     * we decide, I guess, by looking at uio_resid.
	     */
	    int err = ureadc ((int)rbuf[rbufg], uiop);
	    if (err) {
		(void) splx (oldpri);
		if (uiop->uio_resid == 0)
		    return (0);
		return (err);
	    }
	    if (rbufg++ == RBUFSZ)
		rbufg = 0;
	}

	(void) splx (oldpri);

	return (0);
}

/* write to the PC34.
 * we complete it all before returning.
 * commands are short and the device eats chars fast so non-blocking modes
 * aren't worth it.
 */
pc34write(dev_t dev, uio_t *uiop, cred_t *crp)
{
	int oldpri;
	int c;

	oldpri = splx (PC34_SLPPRI);

	/* copy the user buffer to the transmit q and send it.
	 * do it in chunks of our TBUFSZ for efficiency.
	 * keep doing this until no more.
	 */
	while (uiop->uio_resid > 0) {
	    int n, err;

	    /* get min of chars remaining or size of tbuf-1.
	     * N.B. never fill tbuf because then it looks empty!
	     */
	    n = min (uiop->uio_resid, TBUFSZ-1);

	    /* we always just put the message at the front of the buffer. */
	    err = uiomove (tbuf, (long)n, UIO_WRITE, uiop);
	    if (err) {
		(void) splx (oldpri);
		return (err);
	    }
	    tbufg = 0;
	    tbufp = n;

	    /* turning on TBE_E will cause PC34 to ask for next character */
	    pc34_regs.ctrl |= PC34_CTRL_TBE_E;
	    outb (PC34_REG_CTRL, pc34_regs.ctrl);

	    /* sleep until buff is completely sent out */
	    if (sleep (tbuf, PC34_SLPPRI|PCATCH)) {
		/* write interrupted with sleep; discard unsent chars */
		tbufp = tbufg = 0;
		(void) splx (oldpri);
		return (EINTR);
	    }
	}

	(void) splx (oldpri);

	return (0);
}

pc34ioctl (dev_t dev, int cmd, void *arg, int mode, cred_t *crp, int *rvalp)
{
	switch (cmd) {
	case PC34_GET_REGS:
	    /* return the current value of the shadow regs.
	     * get a fresh look at the done bits.
	     */
	    pc34_regs.done = inb (PC34_REG_DONE);
	    if (copyout (&pc34_regs, arg, sizeof(pc34_regs)) < 0)
		return (EFAULT);
	    break;
	default:
	    return (EINVAL);
	}

	return (0);
}

pc34chpoll (dev_t dev, short events, int anyyet, short *revents, struct pollhead **phpp)
{
	if ((events & POLLS) && rbufp != rbufg) {
	    /* we have stuff to be read */
	    *revents = (events & POLLS);
	} else {
	    /* nothing available to be read */
	    *revents = 0;
	    if (!anyyet)
		*phpp = &pc34_ph;
	}

	return (0);
}

pc34intr()
{
	pc34_regs.stat = inb (PC34_REG_STAT);
	pc34_regs.ctrl = inb (PC34_REG_CTRL);

#if USE_DONE
	if (pc34_regs.stat & PC34_STAT_DON_S) {
	    /* TODO: what to do with this done stuff?? */
	    /* a done or error bit has been set */
	    pc34_regs.done |= inb (PC34_REG_DONE);
	    /* reset done bits w/o clearing what the RA and RI commands get */
	    outb (PC34_REG_DATA, 24);	/* see page 7-13 */
	}
#endif

	if (pc34_regs.stat & PC34_STAT_TBE_S) {
	    /* transmit buffer empty -- send more if we have it and wake up
	     * process if no more.
	     */
	    if (tbufp != tbufg) {
		/* we have more to send */
		outb (PC34_REG_DATA, tbuf[tbufg]);	/* send next char */
		if (++tbufg == TBUFSZ)			/* pop off q */
		    tbufg = 0;
	    }
	    if (tbufg == tbufp) {
		/* we have no more to send -- turn off xmit int and wakeup */
		outb (PC34_REG_CTRL, pc34_regs.ctrl & ~PC34_CTRL_TBE_E);
		pc34_regs.ctrl = inb (PC34_REG_CTRL);
		wakeup (tbuf);
	    }
	}

	if (pc34_regs.stat & PC34_STAT_IBF_S) {
	    /* receive buffer full -- put it in the queue (beware of full) and
	     * notify anyone checking for input.
	     */
	    rbuf[rbufp] = inb (PC34_REG_DATA);
	    if (++rbufp == RBUFSZ)
		rbufp = 0;
	    if (rbufp == rbufg) {
		/* buffer is full -- discard oldest char */
		if (++rbufg == RBUFSZ)
		    rbufg = 0;
	    }
	    pollwakeup (&pc34_ph, POLLS);
	    wakeup (rbuf);
	}

	/* the PC's interrupt facility is edge-sensitive only.
	 * the following toggle generates a rising edge if a second interrupt
	 * has occured prior to the first being reset. It works by turning off
	 * interrupts and then back on to create another edge.
	 * see page 7-14.
	 */
	outb (PC34_REG_CTRL, pc34_regs.ctrl & ~PC34_CTRL_IRQ_E);
	outb (PC34_REG_CTRL, pc34_regs.ctrl);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc34.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
