/* include file for the Oregon Micro Systems, Inc, PC34 intelligent motor cntlr.
 */

/* macros to access bits in the four pc34 i/o regs.
 */

/* bits in the "Done Flag" DONE register */
#define	PC34_DONE_X		0x1
#define	PC34_DONE_Y		0x2
#define	PC34_DONE_Z		0x4
#define	PC34_DONE_T		0x8

/* bits in the "Interrupt and DMA Control" CTRL register */
#define	PC34_CTRL_DMA_E		0x01
#define	PC34_CTRL_DMA_DIR	0x02
#define	PC34_CTRL_DON_E		0x10
#define	PC34_CTRL_IBF_E		0x20
#define	PC34_CTRL_TBE_E		0x40
#define	PC34_CTRL_IRQ_E		0x80

/* bits in the "Status" STAT register */
#define	PC34_STAT_CMD_S		0x00
#define	PC34_STAT_INIT		0x01
#define	PC34_STAT_ENC_S		0x02
#define	PC34_STAT_OVRT		0x04
#define	PC34_STAT_DON_S		0x10
#define	PC34_STAT_IBF_S		0x20
#define	PC34_STAT_TBE_S		0x40
#define	PC34_STAT_IRQ_S		0x80

/* pc34 reg set shadow.
 * this is returned from the PC34_GET_REGS ioctl.
 * this is *not* intended to be a memory map template.
 */
typedef struct {
    unsigned char done;
    unsigned char ctrl;
    unsigned char stat;
} PC34_Regs;

/* ioctl commands */
#define	PREFIX		(('3' << 16) | ('4' << 8))
#define	PC34_GET_REGS	(PREFIX|1)

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: pc34.h,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $
 */
