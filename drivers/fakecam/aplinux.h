/* linux include files
 * (C) Copyright 1997 Elwood Charles Downey. All right reserved.
 * 6/25/97
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <asm/segment.h>
#include <asm/io.h>

/* the generic CCD camera interface */
#include "ccdcamera.h"

/* fake out the time() functions in aplow.c */
#define time_t long
#define	time(p)		(*(p) = (time_t)(jiffies/HZ))


/* supply the Linux equivalent word i/o functions */
#define inpw(a)                  (inw(a))
#define outpw(a,v)               (outw((v),(a)))	/* N.B. order */

/* convert a minor device number to an HCCD */
#define	MIN2HCCD(n)	((n)+1)

/* functions from apogeed.c */
extern void ap_pause (int nj);

/* this enum serves to name the indices into the apg*[] module init array.
 * it establishes the order of the module args.
 * see comments elsewhere for their meanings.
 */
typedef enum {
    APP_IOBASE = 0,
    APP_ROWS,
    APP_COLS,
    APP_TOPB,
    APP_BOTB,
    APP_LEFTB,
    APP_RIGHTB,
    APP_TEMPCAL,
    APP_TEMPSCALE,
    APP_TIMESCALE,
    APP_CACHE,
    APP_CABLE,
    APP_MODE,
    APP_TEST,
    APP_16BIT,
    APP_GAIN,
    APP_OPT1,
    APP_OPT2,
    APP_N
} APParams;

/* functions in aplinux.c */
extern int ap_trace;
extern int apProbe(int base);
extern int apInit(int minor, int params[]);
extern int apSetupExposure (int minor, CCDExpoParams *cep);
extern int apReadTempStatus (int minor, CCDTempInfo *tp);
extern int apSetTemperature (int minor, int t);
extern int apCoolerOff (int minor);
extern int apStartExposure(int minor);
extern int apDigitizeCCD (int minor, unsigned short *buf);
extern int apAbortExposure (int minor);
extern void readu_data (HCCD ccd_handle, USHORT *ubuf);

/* in main.c */
extern int ap_impact;

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: aplinux.h,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $
 */
