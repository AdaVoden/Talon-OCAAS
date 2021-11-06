/* driver file for the CyberResearch, Inc, CIO-DIO 8255 I/O board.
 * we support open/close/read/write/ioctl.
 * we only support blocking reads and writes.
 *   read means return all the ports (a/b/c, but not the mode register).
 *   write means set all the ports (a/b/c, but not the mode register).
 * we do not support interrupts.
 * dio_info[] is defined in Space.c. It should have one entry per expected
 *   board. dioinit() will check the boards against this list and adjust
 *   downward. So, you might configure for all DIO96's and just let it go.
 * we maintain no internal software state of any kind so we support multiple
 *   simultaneous processes using this driver at once.
 * N.B. the initialization function leaves all the 8255s in OUTPUT mode.
 *
 * v1.2 12/9/93  turn off the diopresent() checks -- they were commands
 * v1.1 10/28/93 a little more checking in open().
 * v1.0	10/26/93 first working version.
 * v0.1	10/25/93 start work; Elwood C. Downey
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

#include <sys/dio.h>

/* stuff from space.c */
extern int dio_n;		/* number of units, ie, entries dio_info[] */
extern DIOInfo dio_info[];	/* one entry per board, as per System */

/* we are obliged to provide this; see devflag(D1D) */
int diodevflag = 0;

#define	MAXU	3		/* max units we can handle */
#define	BP8255	4		/* bytes per 8255 reg set */

/* given a unit number and a data byte, return the address of that byte */
#define	DIOBA(u,n)	(dio_info[u].io_base + (((n)/3)*BP8255) + ((n)%3))

/* probe each board to see if it seems to match what we configured.
 * if it just comes up short we reset what we think it is.
 */
void
dioinit()
{
	int i, n;

	for (i = 0; i < dio_n; i++) {
	    DIOInfo *cip = &dio_info[i];
	    cmn_err (CE_CONT, "DIO: addr=0x%x expecting model=%d: ",
						    cip->io_base, cip->type);
#ifdef TESTCHIPS
	    n = diopresent (cip);
#else
	    n = cip->type/24;	/* assume just the correct number of chips */
	    cmn_err (CE_CONT, "assuming ");
#endif
	    if (n == cip->type/24)
		cmn_err (CE_CONT, "ready.\n"); /* just what we expect */
	    else {
		cmn_err (CE_CONT, "Base+%d is bad -- ", n*BP8255);
		if (n == 0)
		    cmn_err (CE_CONT, "disabling this unit.\n");
		else
		    cmn_err (CE_CONT, "deferring to a model %d.\n", n*24);
		cip->type = n*24;
	    }
	}
}

/* we really don't have any state to maintain so all we do is make sure
 * the unit number is reasonable and the board has at least 1 working 8255.
 * it's ok to have this driver open by as mant processes at once as you like.
 */
int
dioopen (dev_t *devp, int oflag, int otyp, cred_t *crp)
{
	int unit = geteminor (*devp);
	int n8255;	/* # 8255s working */

	if (unit < 0 || unit > dio_n)
	    return (ENXIO);

	n8255 = dio_info[unit].type/24;
	if (n8255 == 0)
	    return (ENODEV);

	return (0);
}

/* we really don't have any state to restore so this just always reports ok.
 */
dioclose(dev_t dev, int flag, int otyp, cred_t *crp)
{
	return (0);
}

/* read the data bits from the given board.
 * read count must be at least one boards-worth of data bytes, eg, must
 *   read at least 3 bytes from a CIO-DIO 24.
 */
dioread (dev_t dev, uio_t *uiop, cred_t *crp)
{
	int unit = geteminor(dev);	     /* unit number */
	unsigned char data[MAXU * BP8255];   /* contig array for the answer */
	int nbytes = dio_info[unit].type/8;  /* min bytes to read */
	int i;

	/* we set type to 0 in init if first 8255 was bad -- test that here */
	if (nbytes == 0)
	    return (ENODEV);

	/* info.type encodes the number of bits this unit handles.
	 * (it is the "unit" flag in the System file.)
	 */
	if (uiop->uio_resid < nbytes)
	    return (EINVAL);

	/* read the nbytes from the board into our array, data[] */
	for (i = 0; i < nbytes; i++)
	    data[i] = inb (DIOBA(unit, i));

	/* copy back to the user process */
	if (uiomove (data, nbytes, UIO_READ, uiop) != 0)
	    return (EFAULT);

	return (0);
}

/* write the data bits to the given board.
 * write count must exactly match one boards-worth of data bytes, eg, must
 *   write exactly 3 bytes to a CIO-DIO 24.
 */
diowrite(dev_t dev, uio_t *uiop, cred_t *crp)
{
	int unit = geteminor(dev);	     /* unit number */
	unsigned char data[MAXU * BP8255];   /* contig array for the answer */
	int nbytes = uiop->uio_resid;	     /* number of bytes to read */
	int type = dio_info[unit].type;	     /* useful bits */
	int i;

	/* we set type to 0 in init if first 8255 was bad -- test that here */
	if (type == 0)
	    return (ENODEV);

	/* info.type encodes the number of bits this unit handles.
	 * (it is the "unit" flag in the System file.)
	 */
	if (nbytes != type/8)
	    return (EINVAL);

	/* copy the data from the user process */
	if (uiomove (data, nbytes, UIO_WRITE, uiop) != 0)
	    return (EFAULT);

	/* write the nbytes to the board from our array, data[] */
	for (i = 0; i < nbytes; i++)
	    outb (DIOBA(unit, i), data[i]);

	return (0);
}

/* perform a control function to the given board.
 */
dioioctl (dev_t dev, int cmd, void *arg, int mode, cred_t *crp, int *rvalp)
{
	int unit = geteminor (dev);		/* unit number */
	int maxn = dio_info[unit].type/24;	/* max 8255 count */
	volatile int addr;
	DIO_IOCTL dioctl;

	/* get a copy of the user's ioctl request */
	if (copyin (arg, &dioctl, sizeof(dioctl)) < 0)
	    return (EFAULT);

	/* check whether we are asking for a valid set of 8255 regs */
	if (dioctl.n < 0 || dioctl.n >= maxn)
	    return (EXDEV);

	/* do it */
	switch (cmd) {
	case DIO_GET_REGS:	/* get a copy of 8255 reg set dioctl.n */
	    addr = dio_info[unit].io_base + BP8255*dioctl.n;
	    dioctl.dio8255.portA = inb (addr+0);
	    dioctl.dio8255.portB = inb (addr+1);
	    dioctl.dio8255.portC = inb (addr+2);
	    dioctl.dio8255.ctrl  = inb (addr+3);
	    if (copyout (&dioctl, arg, sizeof(dioctl)) < 0)
		return (EFAULT);
	    break;
	case DIO_SET_REGS:	/* set 8255 reg set dioctl.n */
	    addr = dio_info[unit].io_base + BP8255*dioctl.n;
	    outb (addr+3, dioctl.dio8255.ctrl);	/* do the ctrl reg first */
	    outb (addr+0, dioctl.dio8255.portA);
	    outb (addr+1, dioctl.dio8255.portB);
	    outb (addr+2, dioctl.dio8255.portC);
	    break;
	default:
	    return (EINVAL);
	}

	return (0);
}

#ifdef TESTCHIPS
/* probe the unit with DIOInfo *cip.
 * return the number of contiguous 8255s that appear to be there.
 */
static int
diopresent (cip)
DIOInfo *cip;
{
	static unsigned char pattern[] = {0xff, 0x00, 0x5a, 0xa5};
	int max8255 = cip->type/24;	/* 3 8-bit-ports per 8255 */
	int i, j, k;
	int n8255;

	/* idea is to set each port for output mode, which should cause it to
	 * latch whatever we write to it, and test each port of each 8255.
	 * for each 8255 potentially in this unit
	 *   set ports to OUTPUT mode
	 *   for each port (A,B,C)
	 *     for each pattern
	 *       write the pattern to the port
	 *       if it doesn't stick
	 *         know this 8255 is not present so bale out.
	 */
	n8255 = 0;
	for (i = 0; i < max8255; i++) {
	    volatile int addr = cip->io_base + BP8255*i;
	    outb (addr+3, 0x80);	/* sets all outputs to 0 */
	    for (j = 0; j < 3; j++) {
		for (k = 0; k < sizeof(pattern); k++) {
		    unsigned char p, back;
		    p = pattern[k];
		    outb (addr+j, p);
		    back = inb (addr+j);
		    if (back != p)
			goto out;
		}
	    }
	    n8255++;	/* and check for another */
	}
    out:

	return (n8255);
}
#endif /* TESTCHIPS */

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: dio.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
