/*
 * Utility to offset the fits file a number of arcminutes
 * offset will be relative to center if not specified, else relative to
 * RA/DEC values passed in.
 * Will use the CDELT1/CDELT2 values in file header.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "fits.h"
#include "wcs.h"

#define FALSE 0
#define TRUE (!FALSE)

static void usage(char *prognam);
static int doOffset(char *filename, int xoffset, int yoffset, int defCenter, double raRef, double decRef, char *error);

int
main (int ac, char *av[])
{
    char filename[256];
    int defReference = 0;
    double xoffset,yoffset;
    double raRefDeg=0,decRefDeg=0;
    char *prog = *av;
    char *chkp;
    char error[256];
    int rt;

	/* crack arguments */
    if(--ac == 0) usage(prog);
    av++;
    // first argument is file name
    strncpy(filename,*av,sizeof(filename));

    if(--ac == 0) usage(prog);
    av++;
    // second argument is xoffset, in pixels
    xoffset = atoi(*av);

    if(--ac == 0) usage(prog);
    av++;
    // third argument is yoffset, in pixels
    yoffset = atoi(*av);

    if(--ac == 0) {
        // if that's all, then use file RA DEC as reference
        defReference = 1;
    } else {
        av++;
        // otherwise get RA DEC from command line in degrees
        chkp = *av;
        raRefDeg = strtod(chkp,&chkp);
        if(chkp == *av) usage(prog);
        if(--ac == 0) usage(prog);
        av++;
        chkp = *av;
        decRefDeg = strtod(chkp,&chkp);
        if(chkp == *av) usage(prog);
        ac--;
    }

    if(ac != 0) usage(prog);

    rt = doOffset(filename, xoffset, yoffset, defReference, raRefDeg, decRefDeg, error);
    if(!rt) {
        fprintf(stderr,"%s\n",error);
        exit(-1);
    }
    exit(0);
}

static void
usage(char *progname)
{
	FILE *fp = stderr;

	fprintf(fp,"%s: usage: file xoffset yoffset [RACenter, DecCenter]\n", progname);
	fprintf(fp,"  xoffset, yoffset are in pixels\n");
	fprintf(fp,"  RACenter, DecCenter are in degrees\n");
	fprintf(fp,"  If RACenter, DecCenter are not given, they will be taken from file header\n");
	exit (1);
}

static int
doOffset(char *filename, int xoffset, int yoffset, int defCenter, double raRef, double decRef, char *error)
{
    FImage  head, *pHead = &head;
    int     hdrFile;
	double raDelta,decDelta;
	double raCenter,decCenter;
	double dvalue;
	double xrefpix,yrefpix,xrefdeg,yrefdeg,rot;
	char raStr[80];
	char decStr[80];
	char str[80];
	char typestr[80];
	char *type = NULL;	
    double  raDeg,decDeg;
    int projCen1, projCen2;

//printf("doing it: %s %d %d %.4g %.4g %d\n",filename,xoffset,yoffset, raRef, decRef, defCenter);

    // read fits info from file
	if(0 > (hdrFile = open(filename,O_RDWR))) {
		// open error!
		sprintf(error, "Unable to open %s for header info", filename);
		return FALSE;
	}
	if(0 == readFITSHeader (hdrFile, pHead, error)) {
	}
    else {
        return FALSE;
    }	

	// Now get or assume CRVAL1, CRPIX1, CRVAL2, CRPIX2, CROTA2, CTYPE1
	// First, the assumptions...
	rot = 0.0;
    xrefpix = pHead->sw/2;
    yrefpix = pHead->sh/2;
   	xrefdeg = 0;
    yrefdeg = 0;

	if(0 != getRealFITS(pHead,"CDELT1",&raDelta)
	|| 0 != getRealFITS(pHead,"CDELT2",&decDelta)) {
		sprintf(error,"No CDELTn keywords");
    	close(hdrFile);
		return FALSE;
	}

    // get these if available and use as defaults
	if(0 == getStringFITS(pHead,"CTYPE1",typestr)) {
		if (!strncmp (typestr, "RA--", 4)) {	// don't use if we can't identify type
			type = typestr + 4;
		}
	}
	if(0 == getRealFITS(pHead,"CRVAL1",&dvalue)) {
		xrefdeg = dvalue;
	}						
	if(0 == getRealFITS(pHead,"CRPIX1",&dvalue)) {
		xrefpix = dvalue;
	}
	if(0 == getRealFITS(pHead,"CRVAL2",&dvalue)) {
		yrefdeg = dvalue;
	}
	if(0 == getRealFITS(pHead,"CRPIX2",&dvalue)) {
		yrefpix = dvalue;
	}
	if(0 == getRealFITS(pHead,"CROTA2",&dvalue)) {
		rot = dvalue;
	}					

    // if we are using the default center,
    // get these values and use them
    // but only if no WCS solution
    if(defCenter && !type) {			
    	// Ditto for the RA and DEC centers
	    if(0 != getStringFITS(pHead,"RA",raStr)
    	|| 0 != getStringFITS(pHead,"DEC",decStr)) {
	    	sprintf(error,"Missing RA/DEC keyword");
        	close(hdrFile);
		return FALSE;
    	}
	    // and they have to be legible..
    	if(0 != scansex(raStr, &raCenter)
	    || 0 != scansex(decStr, &decCenter))
    	{
	    	sprintf(error,"Unable to extract RA/DEC values");
        	close(hdrFile);
    		return FALSE;
	    }
    	
    	xrefdeg = hrdeg(raCenter);
	    yrefdeg = decCenter;
	    xoffset += (xrefpix+0.5);
	    yoffset += (yrefpix+0.5);
    }
    // if we have passed in values
    // we just use these
    if(!defCenter) {
        // use the values we were given 
        xrefdeg = raRef;
        yrefdeg = decRef;
        // pixel offsets are from raCenter, decCenter as center reference
        projCen1 = -(xoffset-xrefpix);
        projCen2 = -(yoffset-yrefpix);
    } else {
        projCen1 = xrefpix;
        projCen2 = yrefpix;
    }

    if(!type) {
	   type = "-TAN";
    }
			
	// Okay.. on with biz..

//printf("working with xoffset=%d, yoffset=%d\nxrefdeg=%g,yrefdeg=%g\nxrefpix=%g,yrefpix=%g\nradelta=%g, decdelta=%g\nrot=%g\ntype=%s\n",
//	xoffset,yoffset,xrefdeg,yrefdeg,xrefpix,yrefpix,raDelta,decDelta,rot,type);

    // perform worldpos calculation
	/* CRPIXn assume pixels are 1-based */
	(void) worldpos(xrefpix+xoffset, yrefpix+yoffset, xrefdeg, yrefdeg, xrefpix, yrefpix, raDelta, decDelta,
					      rot, type, &raDeg, &decDeg);

    // write the values to the header
    lseek(hdrFile,0,SEEK_SET);

    fs_sexa(str,deghr(raDeg),3,360000); // hh:mm:ss.ss
	setStringFITS(pHead,"RA",str,"Nominal center J2000 RA"); // assume no errors
//printf("ra=%s\n",str);	
	fs_sexa(str,decDeg,3,36000); // ddd:mm:ss.s
	setStringFITS(pHead,"DEC",str,"Nominal center J2000 Dec"); // assume no errors
//printf("dec=%s\n",str);		

    /* Remove other RA/DEC variants that might be here.. they are no longer valid */
    delFImageVar(pHead,"RAEOD");
    delFImageVar(pHead,"DECEOD");
    delFImageVar(pHead,"OBJRA");
    delFImageVar(pHead,"OBJDEC");

    /* Remove any WCS or AMD fields from cropped images -- they must be recalculated!*/
    delWCSFITS(pHead,0);
    /* But put back the CDELT1, 2 keywords */
    setRealFITS(pHead,"CDELT1",raDelta,10,"RA step right, degrees/pixel");
    setRealFITS(pHead,"CDELT2",decDelta,10,"Dec step down, degrees/pixel");

    // record the projection center
	setIntFITS(pHead, "PROJCEN1", projCen1, "X Coordinate of projection center");
	setIntFITS(pHead, "PROJCEN2", projCen2,"Y Coordinate of projection center");					
//printf("proj center %d, %d\n",projCen1, projCen2);

	// Add a comment that says we've offset this
	sprintf(str,"Coordinates set by 'offsetfits' %d, %d from PROJCEN",xoffset,yoffset);
	setCommentFITS(pHead,"HISTORY",str);
		
	// Now write this header out
	writeFITSHeader(pHead, hdrFile, error);
		
	close(hdrFile);
	return TRUE;
		
}










