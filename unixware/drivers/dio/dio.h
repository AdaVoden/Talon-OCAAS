/* include file for the CyberResearch, Inc, CIO-DIO 8255 I/O boards.
 */

/* one of these per unit, defined in space.c.
 */
typedef struct {
	int io_base;	/* I/O base address */
	int io_end;	/* I/O base address */
	int type;	/* type of this board: 24, 48 or 96; this is just the
			 * "unit" field in the System file.
			 * type/24 is often used to compute # of 8255's;
			 * type/8 is often used to compute # of data bytes.
			 */
} DIOInfo;

/* dio 8255 reg set template.
 */
typedef struct {
    unsigned char portA;
    unsigned char portB;
    unsigned char portC;
    unsigned char ctrl;
} DIO8255;

/* ioctl commands */

/* pass a pointer to this as the ioctl arg (3rd arg)
 */
typedef struct {
    int n;		/* which of the several 8255 regs to access */
    DIO8255 dio8255;	/* values for the 8255 regs */
} DIO_IOCTL;

/* pass on of these as the ioctl cmd arg (2nd arg)
 */
#define	PREFIX		(('C' << 16) | ('D' << 8))
#define	DIO_GET_REGS	(PREFIX|1)	/* set 8255 reg set n */
#define	DIO_SET_REGS	(PREFIX|2)	/* get 8255 reg set n */

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: dio.h,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $
 */
