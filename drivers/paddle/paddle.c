/* Linux module to use a gamepad as hand paddle. This is *not* compatable
 * with a typical Linux game program.
 *   compile: cc -O -DPDL_MJR=124 -DMODULE -D__KERNEL__ -c paddle.c
 *   install: insmod paddle.0
 *   mknod:   mknod /dev/paddle c 124 0
 * we support open/close/read/select
 * we allow only one simultaneous open.
 *   
 * Elwood Downey
 * 26 May 98 Begin
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <asm/segment.h>
#include <asm/io.h>

#define	PDL_MAJVER	1		/* major version number */
#define	PDL_MINVER	0		/* minor version number */

static int pdl_io_base = 0x201;		/* i/o address */
#define	PDL_IO_EXTENT	1		/* bytes of I/O space */

static char pdl_present;		/* flag set when we know game exists */
static int xidle, yidle;		/* x and y axes when idle */

#define	INB		(inb(pdl_io_base) & 0xff)
#define	OUTB(b)		(outb(b,pdl_io_base))

static void
pdl_axes (int *xp, int *yp)
{
	unsigned c;
	int x, y;
	int i;

	x = y = 0;
	cli();
	OUTB(0xff);
	for (i = 0; i < 10000 && (!x || !y); i++) {
	    c = INB;
	    if (!(c & 1) && !x)
		x = i;
	    if (!(c & 2) && !y)
		y = i;
	}
	sti();

	*xp = x;
	*yp = y;
}

/* read port setting */
static int
pdl_read (struct inode *node, struct file *file, char *buf, int count)
{
	int x, y;
	int l, r;
	int u, d;
	unsigned c;

	pdl_axes (&x, &y);
printk ("x=%3d y=%3d\n", x, y);
	l = x < xidle/8;
	r = x > 17*xidle/8;
	u = y < yidle/8;
	d = y > 17*yidle/8;

	c = (INB & 0xf0) | (l<<3) | (r<<2) | (u<<1) | (d);

	put_user (c, buf);
	return (1);
}

/* if device is present, just increment module installation count; else return
 * ENODEV.
 */
static int
pdl_open (struct inode *inode, struct file *file)
{
	if (!pdl_present)
	    return (-ENODEV);

	pdl_axes (&xidle, &yidle);
	printk ("xidle=%d yidle=%d\n", xidle, yidle);

	MOD_INC_USE_COUNT;
	return (0);
}    

/* just decrement module installation count */
static void
pdl_release (struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;
}    

/* the master hook into the support functions for this driver */
static struct file_operations pdl_fops = {
    NULL,	/* seek */
    pdl_read,
    NULL,	/* write */
    NULL,	/* readdir */
    NULL,	/* select */
    NULL,	/* ioctl */
    NULL,	/* mmap */
    pdl_open,
    pdl_release,
    NULL,	/* fsync */
    NULL,	/* fasync */
    NULL,	/* check media change */
    NULL,	/* revalidate */
};

/* return non-0 if the game paddle appears to be installed, else 0.
 */
static int
pdl_probe()
{
	return (1);
}

int 
init_module (void)
{
	/* see that our IO ports are not already allocated */
	if (check_region (pdl_io_base, PDL_IO_EXTENT)) {
	    printk ("paddle: IO port 0x%x already in use\n", pdl_io_base);
	    return (-EIO);
	}

	/* probe device */
	if (!pdl_probe()) {
	    printk ("paddle: 0x%x: not present\n", pdl_io_base);
	    return (-ENODEV);
	}

	/* install our handler functions and register the io ports */
	if (register_chrdev (PDL_MJR, "paddle", &pdl_fops) == -EBUSY) {
	    printk ("paddle: major %d is already in use\n", PDL_MJR);
	    return (-EIO);
	}
	request_region (pdl_io_base, PDL_IO_EXTENT, "paddle");
	pdl_present = 1;

	/* ok */
	printk ("paddle at 0x%x: Version %d.%d installed and ready\n",
					pdl_io_base, PDL_MAJVER, PDL_MINVER);

	return (0);
}

void
cleanup_module (void)
{
	unregister_chrdev (PDL_MJR, "paddle");
	release_region (pdl_io_base, PDL_IO_EXTENT);
	printk ("paddle: module removed\n");
	pdl_present = 0;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: paddle.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
