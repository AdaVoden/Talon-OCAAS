/* linux include files and other glue.
 * (C) Copyright 1997 Elwood Charles Downey. All right reserved.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <asm/segment.h>
#include <asm/io.h>

#include "ccdcamera.h"

/* the generic CCD camera interface */
#include "ccdcamera.h"

/* supply the Linux equivalent word i/o functions */
#define inpw(a)                  (inw(a))
#define outpw(a,v)               (outw((v),(a)))	/* N.B. order */

/* max cameras we support */
#define	MAXCAMS		1

/* size of CCD */
#define X_EXTENT        1024
#define Y_EXTENT        1024
#define	MAX_HBIN	X_EXTENT
#define	MAX_VBIN	Y_EXTENT

/* this enum serves to name the indices into the hpc*[] module init array.
 * it establishes the order of the module args.
 * see comments elsewhere for their meanings.
 */
typedef enum {
    HPP_IOBASE = 0,
    HPP_N
} HPParams;

/* in hpc.c */
extern int hpc_trace;
extern int hpcProbe(int base);
extern int hpcInit(int minor, int params[]);
extern int hpcReset(int minor);
extern int hpcSetupExposure (int minor, CCDExpoParams *cep);
extern int hpcReadTempStatus (int minor, CCDTempInfo *tp);
extern int hpcSetTemperature (int minor, int t);
extern int hpcCoolerOff (int minor);
extern int hpcStartExposure(int minor);
extern int hpcDigitizeCCD (int minor, unsigned short *buf);
extern int hpcAbortExposure (int minor);


/* in main.c */
extern int hpc_impact;
extern void hpc_pause (int nj);

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: hpc.h,v $ $Date: 2003/04/15 20:48:09 $ $Revision: 1.1.1.1 $ $Name:  $
 */
