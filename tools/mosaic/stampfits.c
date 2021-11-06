/****************************************************************\

	============================
	Stampfits
	
	A program for making FITS headers and applying them to FITS files.
	
	S. Ohmert June 12, 2001
	(C) 2001 Torus Technologies, Inc.  All Rights Reserved
	
	============================

    This utility will stamp the FITS information from the shared memory data and
    given exposure time to a file, using the -r option.
    This information can then be applied to a FITS image, replacing whatever FITS
    header it originally had with this prerecorded one, using the -a option.
    The RA and Dec coordinates can be extracted from a FITS header and displayed
	as hh:mm:ss hh:mm:ss, with the -c option.
    The filter code can be extracted with the -f option.
	
    This is intended in context with the JSF Pixel Vision camera, where the indirect
    nature of the camera exposure means FITS header information is not available
    directly with the pixel data, and must be rebuilt during post-processing.

    As evolved, it is more relevant to "vcam" (virtual camera) work than for JSF/Pixel Vision.

    See the program "mosaic", which as intended for JSF will use the headers created
    by this utility.
	
	Design goals:
		
		- Callable from script (pvcam). Stamps header info quickly.
		
		- Can also be used to apply header over existing FITS
			(this intended use is superceded by mosaic)
				
	==============================
	
	Caveats:
		
		- Works in concert with camera.cfg
		- Exposure must be given on command line		
		- Will DESTROY any FITS header it is stamped over top of
		
	===============================
	
	To-dos, possibilities, wish list:
	
		
	+++++++++++++++++++++++++++++++++++++
	
	Code Notes:
					
	---------------------------------------		
	
	CVS History:
		$Id: stampfits.c,v 1.1.1.1 2003/04/15 20:48:35 lostairman Exp $
		
		$Log: stampfits.c,v $
		Revision 1.1.1.1  2003/04/15 20:48:35  lostairman
		Initial Import
		
		Revision 1.5  2002/12/21 00:33:30  steve
		timestamp resolution improvement
		
		Revision 1.4  2001/11/16 17:40:24  steve
		Added filter option
		
		Revision 1.3  2001/07/05 19:06:50  steve
		added binning support
		
		Revision 1.2  2001/06/26 03:42:49  steve
		fixed bugs with separated sources & naming
		
		Revision 1.1  2001/06/13 01:57:25  steve
		Add stampfits
		
	
\****************************************************************/	

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "telstatshm.h"
#include "configfile.h"
#include "cliserv.h"
#include "telenv.h"
#include "strops.h"
#include "running.h"
#include "misc.h"
#include "telfits.h"
#include "ccdcamera.h"
#include "tts.h"

// Local functions
static void usage(char *prog);
static int recordInfo(char *filename, int expms, int width, int height, int binx, int biny, char *error);
static int stampInfo(char *recordFile, char *fitsFile, char *error);
static int getcoords(char *filename, char *error);
static int getfilter(char *fitsfile, char * error);
static void init_cfg();
static int getCoolerTemp (CCDTempInfo *tp);
static void addID (FImage *fip);
static void addCDELT (FImage *fip);
static int fitswriteHead (char *fn, FImage *fip, char *error);


// module variables
static TelStatShm *telstatshmp; /* to shared memory segment */

/* orientation and scale info from config file */
static char camcfg[] = "archive/config/camera.cfg";
static double HPIXSZ, VPIXSZ;   /* degrees/pix */
static int CAMDIG_MAX;		/* max time for full-frame download, secs */
static int DEFTEMP;		/* default temp to set, C */
static int LRFLIP, TBFLIP;	/* 1 to flip */
static int RALEFT, DECUP;	/* raw increase */
static char tele_kw[80];	/* TELESCOP keyword */
static char orig_kw[80];	/* ORIGIN keyword */
/* what kind of driver */
static char driver[1024];
static int auxcam;


/* Main entry function -- Command Line access */
int main(int argc, char *argv[])
{
	char s;
	// Get program name
	char *prog = argv[0];
	char error[1024];
	char *precfile;
	int	exp;
	int width,height;
	int binx, biny;
	
	error[0] = 0;
	exp = width = height = 0;
	binx = biny = 1;
	
 	// Crack the arguments
 	while(--argc > 0 && **++argv == '-') {
 		s = (*argv)[1];
 		switch(s) {
 		
 			case '?':
 			case 'h':
 			default:
 				usage(prog);
 				break;
 				
			case 'r':
				++argv;
				precfile = *argv;
				if(--argc > 1)
					exp = atoi(*(++argv));
				if(--argc > 1)
					width = atoi(*(++argv));
				if(--argc > 1)
					height = atoi(*(++argv));
				if(--argc > 1)
					binx = atoi(*(++argv));
				if(--argc > 1)
					biny = atoi(*(++argv));
				if(0==recordInfo(precfile, exp, width, height, binx, biny, error))
					return EXIT_SUCCESS;
				break;
								
			case 'a':
				++argv;
				if(0==stampInfo(*argv, *(argv+1),error))
					return EXIT_SUCCESS;
				break;			
				
			case 'c':
				++argv;
				if(0==getcoords(*argv, error))
					return EXIT_SUCCESS;
				break;				
			case 'f':
				++argv;
				if(0==getfilter(*argv, error))
					return EXIT_SUCCESS;
				break;				
			 				
 		
 		}
 	}
 	if(!error[0])
	 	usage(prog);
	else
		fprintf(stderr,"%s\n",error);
 	
  return EXIT_FAILURE;
}

static void usage(char *prog)
{
	#define	FE fprintf (stderr,

	FE "%s [-r recfile exp <width> <height> <binx> <biny>]\n",prog);
	FE "or %s [-a recfile fitsfile] \n\n", prog);
	FE "Purpose: To record FITS information into a temporary file\n");
	FE "and to apply saved info to a FITS file, replacing any previous FITS information\n\n");
	FE "-h or -?                print this information\n");
	FE "-a <recfile> <fitsfile> apply saved FITS info in <recfile> to fits file named <fitsfile>\n");
	FE "-c <fitsfile>           extract RA and Dec information from FITS header\n");
    FE "-f <fitsfile>           extract filter code from FITS header\n");

	exit (1);	
}

/* Record the shared memory info into a file */
// Return 0 if ok,
// return -1 on error and put error message in error
static int recordInfo(char *filename, int expms, int width, int height, int binx, int biny, char *error)
{
	int shmid;
	long addr;
	FImage fimage, *fip = &fimage;
	CCDTempInfo tinfo;
//	time_t endt;

	// Get shared memory information
	shmid = shmget (TELSTATSHMKEY, sizeof(TelStatShm), 0666|IPC_CREAT);
	if (shmid < 0) {
	    strcpy(error, "shmget TELSTATSHMKEY");
	    return -1;
	}

	addr = (long) shmat (shmid, (void *)0, 0);
	if (addr == -1) {
	    strcpy(error, "shmat TELSTATSHMKEY");
	    return -1;
	}

	telstatshmp = (TelStatShm *) addr;	
	
	// Get config information
	init_cfg();
	
	// Make sure CCDCamera knows about this	
	if (setPathCCD (driver, auxcam, error) < 0)
	    return (-1);
	
	// Create a FITS header from this information
	initFImage (fip);
	
	// Get camera depth, width, height, and exposure time
	// use command line values now, but will replace w/actual image data if we stamp
	fip->bitpix = 16;
	fip->sw = width;
	fip->sh = height;
	fip->bx = binx;
	fip->by = biny;
	// exposure time must come on command line because it's not in PV fits info
	fip->dur = expms;
	
	setSimpleFITSHeader (fip);
	if (orig_kw[0] != '\0')
	    setStringFITS (fip, "ORIGIN", orig_kw, NULL);
	if (tele_kw[0] != '\0')
	    setStringFITS (fip, "TELESCOP", tele_kw, NULL);
	
	addID (fip);	// add camera manufacturer name
	addCDELT (fip);	// add the degrees/pixel step amounts
	
	/* get the time and other stats when the exposure ended */
//	endt = time (NULL);
	timeStampFITS (fip, /*endt*/0, "Time at end of exposure");

	getCoolerTemp (&tinfo);			// camera temp


	addShmFITS (fip, telstatshmp);	// location, weather info, etc.
			
	if(fitswriteHead(filename,fip, error) < 0) {
		return -1;
	}

	return(0);
}

/* Apply the saved shared memory info to a FITS file */
// Return 0 if ok,
// return -1 on error and put error message in error
static int stampInfo(char *recordFile, char *fitsFile, char *error)
{
	FImage fimage;			// fits image
	FImage headinfo;		// new header info
	int rt = -1;  			// assume error
	int fileHdl, hdrFile;	// file handles
	int bitpix,sw,sh,bx,by;

	initFImage (&fimage);
	
	// Load original FITS file
	fileHdl = open(fitsFile, O_RDONLY);
	if(fileHdl == -1) {
		strcpy(error, "unable to read image file");
		return -1;
	}
	// Load Header stamp
	hdrFile = open(recordFile, O_RDONLY);
	if(hdrFile == -1) {
		strcpy(error, "unable to open record file");
		close(fileHdl);
		return -1;
	}
	
	// get fits
	if(0 == readFITS(fileHdl, &fimage, error)) {
	
		// read the values we want to keep from the original image
		bitpix = fimage.bitpix;
		sw = fimage.sw;
		sh = fimage.sh;
		bx = fimage.bx;
		by = fimage.by;
		
		// get header
		if(0 == readFITSHeader (hdrFile, &headinfo, error)) {
			// stamp header over fits
			if(0 == copyFITSHeader(&fimage, &headinfo)) {
				// Now put our saved values back
				fimage.bitpix = bitpix;
				fimage.sw = sw;
				fimage.sh = sh;
				fimage.bx = bx;
				fimage.by = by;
				setSimpleFITSHeader(&fimage);
			
				// SUCCESS!
				rt = 0;
			
			} else {
					// failed to copy for some reason
					strcpy(error, "failed to copy FITS header information");
			}
		} else {
			// failed to read header information
		}
	} else {
		// failed to read fits image
	}
	
	close(hdrFile);
	close(fileHdl);
	
	if(rt == 0) {
	
		// open again for writing
		fileHdl = open(fitsFile, O_WRONLY);
		if(fileHdl == -1) {
			strcpy(error, "unable to write image file");
			return -1;
		}
	
		// write the new version
		rt = writeFITS(fileHdl, &fimage, error, 1);
		
		close(fileHdl);
	}
			
	return (rt);
	
}

/* Extract RA and Dec info from a FITS header */
static int getcoords(char *fitsfile, char * error)
{
	FImage headinfo;		// new header info
	int rt = -1;  			// assume error
	int hdrFile;			// file handles
	char strRA[80];
	char strDec[80];
	
	// Load Header stamp
	hdrFile = open(fitsfile, O_RDONLY);
	if(hdrFile == -1) {
		strcpy(error, "unable to open FITS header file");
		return -1;
	}
	
	// get header
	if(readFITSHeader (hdrFile, &headinfo, error) == 0) {
		if(0==getStringFITS(&headinfo,"RA",strRA)
		&& 0==getStringFITS(&headinfo,"DEC",strDec))
		{
			// OUTPUT TO STDOUT
			printf("%s %s",strRA,strDec);
			rt = 0;
		}
	}
	
	close(hdrFile);
	return(rt);
			
}

/* Extract FILTER code from a FITS header */
static int getfilter(char *fitsfile, char * error)
{
	FImage headinfo;		// new header info
	int rt = -1;  			// assume error
	int hdrFile;			// file handles
	char strFilter[80];
	
	// Load Header stamp
	hdrFile = open(fitsfile, O_RDONLY);
	if(hdrFile == -1) {
		strcpy(error, "unable to open FITS header file");
		return -1;
	}
	
	// get header
	if(readFITSHeader (hdrFile, &headinfo, error) == 0) {
		if(0==getStringFITS(&headinfo,"FILTER",strFilter))
		{
			// OUTPUT TO STDOUT
			printf("%s",strFilter);
			rt = 0;
		}
	}
	
	close(hdrFile);
	return(rt);
			
}


static void init_cfg()
{
#define	NCCFG	(sizeof(ccfg)/sizeof(ccfg[0]))
	static CfgEntry ccfg[] = {
	    {"TELE",	CFG_STR,	tele_kw, sizeof(tele_kw)},
	    {"ORIG",	CFG_STR,	orig_kw, sizeof(tele_kw)},
	    {"VPIXSZ",	CFG_DBL,	&VPIXSZ},
	    {"HPIXSZ",	CFG_DBL,	&HPIXSZ},
	    {"LRFLIP",	CFG_INT,	&LRFLIP},
	    {"TBFLIP",	CFG_INT,	&TBFLIP},
	    {"RALEFT",	CFG_INT,	&RALEFT},
	    {"DECUP",	CFG_INT,	&DECUP},
	    {"DEFTEMP",	CFG_INT,	&DEFTEMP},
	    {"CAMDIG_MAX", CFG_INT,	&CAMDIG_MAX},
	    {"DRIVER",	CFG_STR,	driver, sizeof(driver)},
	    {"AUXCAM",	CFG_INT,	&auxcam},
	};
	int n;

	/* gather config file info */
	n = readCfgFile (0, camcfg, ccfg, NCCFG);
	if (n != NCCFG) {
	    cfgFileError (camcfg, n, NULL, ccfg, NCCFG);
	    exit(1);
	}

	/* we want degrees */
	VPIXSZ /= 3600.0;
	HPIXSZ /= 3600.0;
	
}

/* fetch and save cooler temp */
static int getCoolerTemp (CCDTempInfo *tp)
{
	char errmsg[1024];

	if (getTempCCD (tp, errmsg) < 0) {
	    daemonLog ("%s", errmsg);
	    return (-1);
	}

	telstatshmp->camtemp = tp->t;
	telstatshmp->coolerstatus = tp->s;

	return (0);
}

/* add the name of the camera manufaturer as the INSTRUME string */
static void addID (FImage *fip)
{
	char buf[256], err[1024];

	if (getIDCCD (buf, err) == 0)
	    setStringFITS (fip, "INSTRUME", buf, NULL);
}

/* figure out the right signs for CDELT{1,2} */
static void addCDELT (FImage *fip)
{
	int sign;

	sign = LRFLIP == RALEFT ? 1 : -1;
	setRealFITS (fip, "CDELT1", HPIXSZ*fip->bx * sign, 10,
						"RA step right, degrees/pixel");

	sign = TBFLIP == DECUP  ? 1 : -1;
	setRealFITS (fip, "CDELT2", VPIXSZ*fip->by * sign, 10,
						"Dec step down, degrees/pixel");
}

/* (over)write fn with fip.
 * return 0 if ok else -1.
 */
static int fitswriteHead (char *fn, FImage *fip, char * error)
{
	char *bn = basenm (fn);
	char errmsg[1024];
	int s, fd;

	fd = open (fn, O_RDWR | O_TRUNC | O_CREAT, 0666);
	if (fd < 0) {
	    sprintf (error, "%s: %s\n", bn, strerror(errno));
	    resetFImage (fip);
	    return (-1);
	}
	s = writeFITSHeader (fip, fd, errmsg);
	(void) close (fd);
	resetFImage (fip);
	if (s < 0) {
	    sprintf (error, "%s: %s\n", bn, errmsg);
	    return (-1);
	}

	/* ok */
	return (0);
}

