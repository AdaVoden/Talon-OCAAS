/* Linux driver module for the CyberResearch, Inc, CIO-DIO 8255 I/O board.
 * (C) Copyright 1997 Elwood Charles Downey. All rights reserved.
 * we support up to 4 boards in any combination of the 24/48/96 models.
 * we have only tested this using Mode 0 I/O.
 *
 *  4 Dec 97 Begin
 *  4 Dec 97 Done
 *
 *  compile: cc -O -DDIO_MJR=122 -DMODULE -D__KERNEL__ -c dio.c
 *  install: insmod dio.0 dio=base,bits[,base,bits]
 *  mknod:   mknod dev/dioXX c 122 0
 *  mknod:   mknod dev/dioYY c 122 1
 *
 * for example, dio96 @ 0x300, dio24 @ 0x350:
 *  insmod dio.o dio=0x300,96,0x350,24
 *  mknod dev/dio96 122 0
 *  mknod dev/dio24 122 1
 *
 * N.B. no attempt is made to verify the parameters! The only way I could
 *   think of to detect a board without requiring a loopback plug is to set the
 *   8255's to output and see if they would latch their data values, but this
 *   might exite the connected device in bad ways.
 *   
 * we support open/close/read/write/ioctl:
 * open validates the minor device number, checks for r/w, and increments the
 *   module usage count.
 * close just decrements the module usage count.
 * read returns all the current data registers for all 8255's on the given
 *   board, one per byte, starting with A on the lowest addressed 8255. The
 *   buffer supplied must be at least large enough to hold all the data
 *   registers on a given board. The return value is the actual number of bytes
 *   (registers) returned (and so can be used to deduce the number of chips, by
 *   dividing by 3 (not 4, since the ctrl register is not included in the read
 *   data)).
 * write sets all the data ports on all 8255's on the given board from the
 *   supplied buffer, one per byte, starting with A on the lowest addressed
 *   8255. The length of the buffer supplied must match the total number of
 *   data registers on the given board.
 * ioctl can read or write all 4 registers on any one 8255; it uses the
 *   DIO_IOCTL struct. "n" in that struct is the chip number, with 0 to mean
 *   the lowest addressed 8255.
 * we allow any number of simultaneous processes to use this driver at once
 *   with no attempt to keep things sensible; if they don't cooperate they can
 *   interfere.
 * no interrupts.
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
#include <asm/segment.h>
#include <asm/io.h>

#include "dio.h"

#define	DIOMaxN 	4		/* max boards we can handle */

/* END OF USER SETTABLE OPTIONS */

#define	DIO_MAJVER	1		/* driver major version number */
#define	DIO_MINVER	0		/* driver minor version number */

/* info about one board -- filled in my insmod params */
typedef struct {
    int dio_base;			/* base io address */
    int dio_bits;			/* bits on board */
} DIOInfo;
static DIOInfo dio[DIOMaxN];

#define DIOBP8255  	4			 /* bytes per 8255 reg set */
#define	DION8255(dp)	(((dp)->dio_bits)/24)	 /* # of 8255's */
#define	DIONDATA(dp)	(((dp)->dio_bits)/8)	 /* # of data bytes */
#define	DIONADDR(dp)	(DION8255(dp)*DIOBP8255) /* # addresses used */
#define	DIOM8255PB	4			 /* max 8255's per board */

/* shadow the control register, since can't read it back */
static unsigned char dio_csh[DIOMaxN*DIOM8255PB];
#define	CSHPO		0x9b			/* ctrl reg at powerup/reset */

/* given a minor number and a data byte cardinal, return address of that byte */
#define DIOBA(m,n)      (dio[m].dio_base + (((n)/3)*DIOBP8255) + ((n)%3))

static char dio_name[] = "dio";		/* name known to module tools */

/* system call interface to read into buf[count].
 * read the data bits from the given board.
 * read count must be at least one boards-worth of data bytes, eg, must
 *   read at least 3 bytes from a CIO-DIO 24.
 */
static int
dio_read (struct inode *node, struct file *file, char *buf, int count)
{
	int minor = MINOR (node->i_rdev);
	DIOInfo *dp = &dio[minor];
	int nbytes = DIONDATA(dp);	/* actual bytes needed */
	int i;

	/* qualify size and access */
	if (count < nbytes)
	    return (-EINVAL);
	i = verify_area(VERIFY_WRITE, (const void *)buf, nbytes);
	if (i)
	    return (i);

	/* read the nbytes from the board and pass back to user */
	for (i = 0; i < nbytes; i++) {
	    int addr = DIOBA(minor, i);
	    unsigned char byte = inb_p (addr);
	    put_user (byte, buf);
	    buf++;
	}

	/* ok */
	return (nbytes);
}

/* system call interface to write buf[count] to the device.
 * write the data bits to the given board.
 * write count must exactly match one boards-worth of data bytes, eg, must
 *   write exactly 3 bytes to a CIO-DIO 24.
 */
static int
dio_write (struct inode *node, struct file *file, const char *buf, int count)
{
	int minor = MINOR (node->i_rdev);
	DIOInfo *dp = &dio[minor];
	int nbytes = DIONDATA(dp);	/* actual bytes needed */
	int i;

	/* qualify size and access */
	if (count != nbytes)
	    return (-EINVAL);
	i = verify_area(VERIFY_READ, (const void *)buf, nbytes);
	if (i)
	    return (i);

	/* write users nbytes to the board */
	for (i = 0; i < nbytes; i++) {
	    unsigned char byte = get_user (buf);
	    int addr = DIOBA(minor, i);
	    outb_p (byte, addr);
	    buf++;
	}

	/* ok */
	return (nbytes);
}

/* system call interface for ioctl.
 */
static int
dio_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
unsigned long arg)
{
	int minor = MINOR (inode->i_rdev);
	DIOInfo *dp = &dio[minor];
	DIO_IOCTL dioctl;
	int s, addr;

	/* get a copy of the user's ioctl request */
	s = verify_area(VERIFY_READ, (const void *)arg, sizeof(dioctl));
	if (s)
	    return (s);
	memcpy_fromfs((void *)&dioctl, (const void *)arg, sizeof(dioctl));

	/* check whether we are asking for a valid set of 8255 regs */
	if (dioctl.n < 0 || dioctl.n >= DION8255(dp))
	    return (-ENXIO);

	/* set up address to base of 8255 dioctl.n */
	addr = dp->dio_base + DIOBP8255*dioctl.n;

	/* proceed */
	switch (cmd) {
	case DIO_GET_REGS:      /* get a copy of 8255 reg set dioctl.n */
	    s = verify_area(VERIFY_WRITE, (const void *)arg, sizeof(dioctl));
	    if (s)
		return (s);
	    dioctl.dio8255.portA = inb_p (addr+0);
	    dioctl.dio8255.portB = inb_p (addr+1);
	    dioctl.dio8255.portC = inb_p (addr+2);
	    dioctl.dio8255.ctrl  = dio_csh[(DIOM8255PB*minor)+dioctl.n];
	    memcpy_tofs((void *)arg, (void *)&dioctl, sizeof(dioctl));
	    break;

	case DIO_SET_REGS:      /* set 8255 reg set dioctl.n */
	    dio_csh[(DIOM8255PB*minor)+dioctl.n] = dioctl.dio8255.ctrl;
	    outb_p (dioctl.dio8255.ctrl, addr+3); /* do the ctrl reg first */
	    outb_p (dioctl.dio8255.portA, addr+0);
	    outb_p (dioctl.dio8255.portB, addr+1);
	    outb_p (dioctl.dio8255.portC, addr+2);
	    break;

	default:
	    return (-EINVAL);
	}

	/* ok */
	return (0);
}

/* if device is present, just increment module installation count; else return
 * ENODEV.
 */
static int
dio_open (struct inode *inode, struct file *file)
{
	int minor = MINOR (inode->i_rdev);
	int flags = file->f_flags;

	if (minor < 0 || minor >= DIOMaxN || !dio[minor].dio_base)
	    return (-ENODEV);
	if ((flags & O_ACCMODE) != O_RDWR)
	    return (-EACCES);

	MOD_INC_USE_COUNT;
	return (0);
}    

/* just decrement module installation count */
static void
dio_release (struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;
}    

/* the master hook into the support functions for this driver */
static struct file_operations dio_fops = {
    NULL,	/* lseek */
    dio_read,
    dio_write,
    NULL,	/* readdir */
    NULL,	/* select */
    dio_ioctl,
    NULL,	/* mmap */
    dio_open,
    dio_release,
    NULL,	/* fsync */
    NULL,	/* fasync */
    NULL,	/* check media change */
    NULL,	/* revalidate */
};

/* called by Linux when we are installed */
int 
init_module (void)
{
	int nok;
	int i;

	/* scan all possible minor numbers.
	 * N.B. don't commit linux resources until after the loop.
	 */
	for (nok = i = 0; i < DIOMaxN; i++) {
	    DIOInfo *dp = &dio[i];
	    int base = dp->dio_base;
	    int bits = dp->dio_bits;
	    int chip;

	    /* skip if no base address defined */
	    if (!base)
		continue;

	    /* qualify bits */
	    if (bits != 24 && bits != 48 && bits != 96) {
		printk ("%s at 0x%x: bits set to %d but must be 24, 48 or 96\n",
							dio_name, base, bits);
		dp->dio_base = 0;	/* mark as unuseable */
		continue;
	    }

	    /* see whether these IO ports are already allocated */
	    if (check_region (base, DIONADDR(dp))) {
		printk ("%s: 0x%x already in use\n", dio_name, base);
		dp->dio_base = 0;	/* mark as unuseable */
		continue;
	    }

	    /* set default control register shadow.
	     * N.B. we assume the chips are in a reset state
	     */
	    for (chip = 0; chip < DION8255(dp); chip++)
		dio_csh[(DIOM8255PB*i)+chip] = CSHPO;

	    /* this board looks reasonable */
	    nok++;
	}

	if (nok == 0) {
	    printk ("%s: no boards configured.\n", dio_name);
	    return (-EINVAL);
	}

	/* install our handler functions -- DIO_MJR might be in use */
	if (register_chrdev (DIO_MJR, dio_name, &dio_fops) == -EBUSY) {
	    printk ("%s: major %d already in use\n", dio_name, DIO_MJR);
	    return (-EIO);
	}

	/* ok, log and register the io regions as in use with linux */
	for (i = 0; i < DIOMaxN; i++) {
	    DIOInfo *dp = &dio[i];
	    int base = dp->dio_base;
	    int bits = dp->dio_bits;

	    if (!base)
		continue;

	    printk("%s%d at 0x%x: Not probed -- assumed ok. Version %d.%d\n",
				dio_name, bits, base, DIO_MAJVER, DIO_MINVER);
	    request_region (base, DIONADDR(dp), dio_name);
	}

	/* all set to go */
	return (0);
}

/* called by Linux when module is uninstalled */
void
cleanup_module (void)
{
	int i;

	for (i = 0; i < DIOMaxN; i++) {
	    DIOInfo *dp = &dio[i];
	    int base = dp->dio_base;

	    if (!base)
		continue;

	    release_region (base, DIONADDR(dp));
	}

	unregister_chrdev (DIO_MJR, dio_name);
	printk ("%s: module removed\n", dio_name);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: dio.c,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $"};
