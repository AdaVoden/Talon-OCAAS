/* include file for the Spectra Source Instruments' HPC-1 CCD Camera.
 * much of this code is directly modeled after that in cam215b.h.
 */

/* ioctl commands */
#define	PREFIX		(('h' << 16) | ('p' << 8))
#define	HPC_SET_EXPO	(PREFIX|1)	/* arg = *HPCExpoParams */
#define	HPC_SET_COOLER	(PREFIX|2)	/* arg = 1 for on, 0 for off */
#define	HPC_TRACE	(PREFIX|3)	/* arg = 1 for on, 0 for off */
#define	HPC_READ_TEMP	(PREFIX|4)	/* set arg to AD2 */

/* use this with HPC_SET_EXPO ioctl */
typedef struct {
    int bx, by;		/* x and y binning */
    int sx, sy, sw, sh;	/* subimage upper left location and size */
    int duration;	/* ms to expose */
    int shutter;	/* whether to open the shutter during the exposure. */
} HPCExpoParams;

/* size of CCD */
#define	X_EXTENT	1024
#define	Y_EXTENT	1024

/* -- Camera control bits -- */
#define CLOCK_MASK          0x0020
#define TEC_ENABLE_MASK     0x0040
#define TEC_INIT_MASK       0x0080
#define CCD_RESET_MASK      0x0100
#define SHUTTER_MASK        0x0200

#define DEFAULT_BASE_PORT 	0x120

#define RESTART			0x001F
#define SER2_16			0x001E
#define SER2_8			0x001D
#define HBIN1			0x001C
#define HBIN2			0x001B
#define ABG			0x001A
#define INTEGRATE		0x0019

#define OPAR			0x000F
#define EPAR			0x000E
#define FTEPAR			0x000D
#define BPAR			0x000C
#define FTBPAR			0x000B

#define NUM_DUMMIES		24
#define PARALLELPIXELS		80


/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: hpc.h,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $
 */
