/****************************************************************\

	============================
	Mosaic
	
	A program for cutting apart and reassembling FITS files.
	
	S. Ohmert June 12, 2001
	(C) 2001 Torus Technologies, Inc.  All Rights Reserved
	
	============================
	
	Designed for general use, but with specific attention
	to use as a post-process tool for Pixel Vision camera
	implementations at Bisei Spaceguard Center.
	
	Design goals:
		
		- Efficient throughput. Files are read/written as streams
		  in a single pass and images are not brought into memory.
		
		- Cut up and recombine maintaining proper coordinate information.
		  Able to solve for WCS solutions in whole or in part.
		
		- Works with separated Header / Image sources as required for
		  PV camera context.
		
	==============================
	
	Caveats:
		
		- FITS headers are expected to have certain fields.
		- Pixel information must be left-to-right / top-down.
		- Uncompressed FITS only
		
	===============================
	
	To-dos, possibilities, wish list:
	
		- Search for TODO in source for comments
		- Astronomical coordinate mapping		
		- Need to adjust other coordinates, such as RAEOD, DECEOD?
		- Should we check / require same filter, exposure, etc. for composites?
		- More header info in mosaic images?  What?
		- ? ? ? ?
		
	+++++++++++++++++++++++++++++++++++++
	
	Code Notes:
	
		Not the worlds most elegant code. Quite linear, global.
		Some redundant copy-and-paste areas.
		Knocked out quickly.
		Straightforward, though, should be relatively easy to follow.
				
	---------------------------------------		
	
	CVS History:
		$Id: mosaic.c,v 1.1.1.1 2003/04/15 20:48:35 lostairman Exp $
		
		$Log: mosaic.c,v $
		Revision 1.1.1.1  2003/04/15 20:48:35  lostairman
		Initial Import
		
		Revision 1.19  2002/11/15 17:04:19  steve
		fixed bug that was truncating -r processed images to 8-bit resolution
		
		Revision 1.18  2002/10/16 03:55:42  steve
		debug/update
		
		Revision 1.17  2002/10/14 21:58:04  steve
		minor fix
		
		Revision 1.16  2002/10/14 21:39:31  steve
		added projcen support and updated usage documentation.  Replaced & with + as action combine.
		
		Revision 1.14  2002/10/02 15:52:16  steve
		Changed to using worldpos for offsets
		
		Revision 1.13  2002/09/27 21:07:09  steve
		more work with offsets
		
		Revision 1.10  2002/09/20 04:01:58  steve
		bug with proj. center
		
		Revision 1.9  2002/09/20 03:10:15  steve
		added PROJCENx keywords to support new WCS changes
		
		Revision 1.8  2002/02/17 13:58:22  steve
		adjusted ra offset by cos of dec
		
		Revision 1.7  2001/07/06 00:36:51  steve
		removed requirement for image scale to match
		
		Revision 1.6  2001/07/05 19:07:44  steve
		adjust other RA, Dec variants
		
		Revision 1.5  2001/06/29 21:34:05  steve
		added support for suffix naming
		
		Revision 1.4  2001/06/27 05:41:17  steve
		Fixed bug: RA delta convert from degrees to hours
		
		Revision 1.3  2001/06/26 03:42:49  steve
		fixed bugs with separated sources & naming
		
		Revision 1.2  2001/06/13 00:39:14  steve
		Test cvs log update
		
		Revision 1.1  2001/06/13 00:36:43  steve
		First version of mosaic added to CVS
		
		
	
\****************************************************************/	


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "strops.h"
#include "wcs.h"

#include "rectangles.h"

//-------------------------------------------------------

// local prototypes for this program
static void cantdoboth(char *prog);
static void mustbecombining(char *prog, char s);
static void usage(char *prog);
static void errorExit(void);

static int getPAWidth(void);
static int getPAHeight(void);
static void addOrientationSection(int x, int y, int width, int height, char *action, int destx, int desty);
static int closeImg(int imgFile);
static int readImg(int imgFile, void *buf, int len);
static BOOL isOrientation();
static void streamIntoOrientation(int imgFile, int w, int h);
static void arrangeIntoBuffer(void);
static BOOL createPixArrangeBuffer(char *error);
static void closeOrientation(void);
static BOOL initOrientation(char *error);

//static void addAstroRect(char *eastRA, char *northDec, char *westRA, char *southDec);
static void addPixelRect(int x, int y, int width, int height);

static BOOL initSections(char *error);
static BOOL initSection(int section, FImage *pHead, char *error);
static BOOL streamIntoSections(char * error);
static void closeAllSections(void);

static BOOL addComponentFITS(char *filename, char *error);
static BOOL createMosaicFITS(char * error);

// Module scope variables
static char *headerfile;		// name of file used for header
static char *imagefile;			// name of file used for image
static char *outfile;			// name of resulting mosaic file
static char basename[1024];		// basename used to create numbered section files
static long byteOffset;			// offset into imagefile for pixel data
static Rect constraint;			// clipping rectangle applied to mosaic output
static BOOL jflag;				// true if we've set projCen1,2 manually
static int  projCen1;			// x value of center of FOV if jflag is true
static int  projCen2;			// y value of center of FOV if jflag is true

// Values tracked for the mosaic being created
static double mosaicRARefDeg;	
static double mosaicDecRefDeg;
static int mosaicXRefPix;
static int mosaicYRefPix;
static double mosaicRADelta;
static double mosaicDecDelta;
static double mosaicRot;
static char mosaicType[16];
static Rect mosaicRect;
static int mosaicBitpix;
static int mosaicBinX;
static int mosaicBinY;

// reference header info
static FImage headinfo;

// Define a file section used when cutting
typedef struct
{
	int	fileHdl;
	Rect rect;
} FSect;

// Define a maximum number of sections.  We will be limited anyway
// because of number of open file handles, etc. so there's no real
// need for dynamic arrays here.
#define MAX_FSECT	16

// Create an array of MAX_SECT size for all the subsections
FSect sectList[MAX_FSECT];
int numSect;
char * suffix[MAX_FSECT]; // optional suffixes. strdup strings must be freed.


/*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*/

/* Main entry function -- Command Line access */
int main(int argc, char *argv[])
{
	char s;
	// Get program name
	char *prog = basenm(argv[0]);
	char error[1024];
	BOOL cutting = FALSE;
	BOOL combining = FALSE;
	BOOL processed = FALSE;
	char *filename;
	int i;
	
	imagefile = headerfile = outfile = NULL;
	
	error[0] = 0;
	basename[0] = 0;
	
	// insure this is clear when we start 'cause we'll free it later..
	for(i=0; i<MAX_FSECT; i++) {
		suffix[i] = (char *) NULL;
	}
		
	// no sections yet
	numSect = 0;
	
	// byte offset is zero if not explicitly specified or implied
	byteOffset = 0;
	
	// set constraint to all zeros to indicate no constraint
	SetRect(&constraint, 0,0,0,0);
	
 // Crack the arguments
 while(--argc > 0) {
 	if(**(argv+1) == '-')
 	{
 		argv++;
 		s = (*argv)[1];
 		switch(s) {
 		
 			case '?':
 			default:
 				usage(prog);
 				break;
 				
/*			case 'a':
				if(combining) cantdoboth(prog);
				cutting = TRUE;
				++argv;
				if(argc > 3) {
					addAstroRect(*argv,*(argv+1),*(argv+2),*(argv+3));
					argc -= 4;
					argv += 3;
				}
				break;
*/				
			case 'r':
				if(combining) cantdoboth(prog);
				++argv;
				if(argc > 7) {
					addOrientationSection(atoi(*argv),atoi(*(argv+1)),atoi(*(argv+2)),atoi(*(argv+3)),
									      *(argv+4), atoi(*(argv+5)), atoi(*(argv+6)));
					argc -= 7;
					argv += 6;									
				}
				break;
				
			case 'p':
				if(combining) cantdoboth(prog);
				cutting = TRUE;
				++argv;
				if(argc > 3) {
					addPixelRect(atoi(*argv),atoi(*(argv+1)),atoi(*(argv+2)),atoi(*(argv+3)));
					argc -= 4;
					argv += 3;
				}
				break;			
				
 			case 'h':
				if(combining) cantdoboth(prog);
				cutting = TRUE;
				++argv;
				headerfile = *argv;
				argc--;
				break;
				
			case 'i':
				if(combining) cantdoboth(prog);
				cutting = TRUE;
				++argv;
				imagefile = *argv;
				if(!basename[0]) strcpy(basename,basenm(imagefile));
				argc--;
				break;
				
			case 'b':
				if(combining) cantdoboth(prog);
				cutting = TRUE;
				++argv;
				byteOffset = atol(*argv);
				argc--;
				break;				
 				
			case 'n':
				if(combining) cantdoboth(prog);
				cutting = TRUE;
				++argv;
				--argc;
				strcpy(basename,*argv);
				break;
				
			case 's':
				if(combining) cantdoboth(prog);
				cutting = TRUE;
				++argv;
				--argc;
				{
					// copy suffixes listed into suffix string list
					char *t;
					t = strtok(*argv,",");
					i = 0;
					while(t != NULL) {
						suffix[i] = strdup(t);
						i++;
						t = strtok(NULL,",");
					}
					suffix[i] = NULL;
				}
				break;
				
			case 'm':
				if(cutting) cantdoboth(prog);
				combining = TRUE;
				++argv;
				outfile = *argv;
				argc--;
				break;
												
/*			case 'A':
				if(!combining) mustbecombining(prog, s);
				++argv;
				// TODO -- give constraining rect
				printf("WARNING: Constraint feature not complete! NO clipping will occur!\n");
				argc -= 4;
				argv += 3;
				break;
*/				
			case 'P':
				if(!combining) mustbecombining(prog, s);
				++argv;
				SetRectWH(&constraint, atoi(*argv),atoi(*(argv+1)),atoi(*(argv+2)),atoi(*(argv+3)));
				argc -= 4;
				argv += 3;
				break;							 				
				
			case 'j':
				if(combining) cantdoboth(prog);
				++jflag;
				++argv;
				projCen1 = atoi(*argv);
				projCen2 = atoi(*(argv+1));
				argc -= 2;
				argv += 1;
				break;
 		}
 	}
 	else
 	{
	 	// not an option, must be file name
 		filename = *(++argv);
 		if(combining)
 		{
 			// if combining, bring this one into the party
 			if(!addComponentFITS(filename, error))
 				break;
 		}
 		else
 		{
 			cutting = TRUE;
 			if(!headerfile) headerfile = filename;
 			if(!imagefile) imagefile = filename;
 			if(!basename[0]) strcpy(basename, basenm(imagefile));
 		}
	 }
  }

  // see if we have everything
  if(cutting)
  {
  	if(headerfile && imagefile)
  	{
  		// fix up basename to remove extension if it has one
  		int i = strlen(basename) - 4;
  		if(i > 0) {
  			if(basename[i] == '.')
  				basename[i] = '\0';
  		}

  		if(initOrientation(error)) {  		  		
	  		if(initSections(error)) {			// create sections, and init using headerfile base info
		  		if(streamIntoSections(error)) {	// stream pixel data and dispatch into sections
			  		processed = 1;
			  	}
			}
		}
		closeOrientation();						
		closeAllSections();
	}
	
  }
  else if(combining)
  {
  	if(error[0]) { // error from adding component or other
  		processed = 0;
  	}
  	else {
	  	processed = createMosaicFITS(error);
  	}
  	
  	closeAllSections();
  }

  if(!processed)
  {
	  if(!error[0])
		 usage(prog);
	  else
		fprintf(stderr,"%s\n",error);
 	
	  return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static void errorExit()
{
	closeAllSections();
	exit(1);
}

static void cantdoboth(char *prog)
{
	fprintf(stderr,"Can't specify -m and also -a or -p\n");
	usage(prog);
}
static void mustbecombining(char *prog, char s)
{
	fprintf(stderr,"Can't use -%c option unless you first specify -m\n",s);
	usage(prog);
}

static void usage(char *prog)
{
	#define	FE fprintf (stderr,
	
	FE "\n");
//	FE "%s [-r <orientation parameters> ...] [-a <aoi rect> -p <pix rect> ... ] fitsfile \n", prog);
//	FE "or %s [-r <orientation parameters> ...] [-a <aoi rect> -p <pix rect> ... ] -h headerfile -i imagefile \n", prog);
	FE "%s [-r <orientation parameters> ...] [-p <pix rect> ... ] fitsfile \n", prog);
	FE "or %s [-r <orientation parameters> ...] [-p <pix rect> ... ] -h headerfile -i imagefile \n", prog);
	FE "or %s -m outfile files...\n\n",prog);
	FE "ORIENTATION:\n");
	FE "  -r       Orientation directives must occur first, if used.\n");
	FE "           Their purpose is to rearrange the incoming pixels into an image\n");
	FE "           which is properly flipped and placed with respect to the specified header.\n");
	FE "           The format for the orientation parameters is:\n");
	FE "              \"x y width height action destx desty\"\n");
	FE "           where \"x y width height\" identifies a rectangular area within the source pixels\n");
	FE "           and \"action\" describes how to affect these pixels before placing the result\n");
	FE "           at \"destx desty\" of the image array that will be presented for subsequent cutting.\n");
	FE "           The \"action\" commands may be one of the following:\n");
	FE "               none             Do no flip or transpose\n");
	FE "               mirror           Horizontal mirror image\n");
	FE "               turnCW           Rotate 90 degrees right, transposing x/y axes\n");
	FE "               turnCCW          Rotate 90 degrees left, transposing x/y axes\n");
	FE "               turnOver         Rotate 180 degrees\n");
	FE "               turnCW+mirror    Rotate CW then mirror\n");
	FE "               turnCCW+mirror   Rotate CCW then mirror\n");
	FE "               turnOver+mirror  Rotate 180 degrees then mirror (i.e. vertical flip)\n");
	FE "           More than one -r directive may be given, allowing for complex rearrangement of pixels\n");
	FE "           (e.g. PixelVision camera output and BSGC usage)\n");
	FE "           A single -r directive can be useful in presenting a flipped image in the correct\n");
	FE "           North-at-top, East-at-left orientation required for Mosaic to work properly\n");
	FE "\n");
	FE "CUTTING:\n");
/*	
    FE "  -a       Sections all images to the given astronomical AOI.\n");
    FE "           The AOI is in the form: \"East_RA North_Dec West_RA South_Dec\",\n");
    FE "           RA as H:M:S, Dec as D:M:S.\n");
    FE "           Repeat option for any number of areas to crop\n");
    FE "\n");
*/
    FE "  -p       Sections images by pixel rectangles.\n");
    FE "           The form is: \"x y width height\"\n");
    FE "           Repeat option for any number of areas to crop\n");
	FE "\n");
	FE "  -n       Specify a base name for the output files.\n");
	FE "           By default, this is the imagefile name (or fitsfile)\n");
	FE "           Base name is suffixed with nn for each constituent section file created\n");
	FE "\n");
	FE "  -s       Specify suffix names, as a list of comma-delimited items (no spaces)\n");
	FE "           By default, these are 00,01,02.  Names may be descriptive. ex: -s NW,NE,SW,SE \n");
//	FE "  -f       Specify a file that contains sections, one per line.
//	FE "\n");
	FE "  -h file  Specify the FITS header file to apply as reference\n");
	FE "  -i file  Specify the source of image pixels\n");
	FE "  -b bytes Specify offset into imagefile where pixel data starts\n");
	FE "  -j x y   Specify the PROJCEN1 and PROJCEN2 origin reference.\n");
	FE "           From the upper left of the image (or orientation rect), this is the pixel coordinate\n");
	FE "           of the center of the field of view.  Default is center of source image.\n");
	FE "\n");
	FE "COMBINING:\n");
	FE "  -m file  Create a mosaic FITS image out of listed files.\n");
	FE "           Save the resulting mosaic image in named file\n");
	FE "\n");
/*	
	FE "  -A       Specify a constraining (clipping) AOI for mosaic.  In the form:\n");
    FE "           \"East_RA North_Dec West_RA South_Dec\", RA as H:M:S, Dec as D:M:S.\n");
	FE "\n");
*/	
	FE "  -P       Specify a constraining (clipping) pixel rectangle for output mosaic,\n");	
    FE "           in the form \"x y width height\" \n");
    FE "\n");
	FE "\n");
	FE "Notes:\n");
	FE "Combining images will work best if constituent images have first been WCS corrected\n");
	FE "Images cut from a larger image will NOT have WCS fields added\n");
	FE "Image presented for cutting MUST have North at top and East at left.  The program does not\n");
	FE "check for this or issue any error or warning.  Use the -r directives to pre-orient image if needed.\n");
	FE "\n");	
	errorExit();	
}

/*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*/

/*
static void addAstroRect(char *eastRA, char *northDec, char *westRA, char *southDec)
{
	printf("Astro Rect: %s, %s  --> %s, %s\n",eastRA,northDec,westRA,southDec);
	printf("Despite documentation, this isn't supported\n");
	errorExit();
}
*/
// Add a rectangle
static void addPixelRect(int x, int y, int width, int height)
{	
//	printf("Pixel Rect: %d, %d  %d x %d\n",x,y,width,height);
	
	SetRectWH(&sectList[numSect].rect,x,y,width,height);
	sectList[numSect].fileHdl = -1;
	numSect++;
}

/***************************\
 	initSections()
 	
 This creates all the subfiles for each region
 and writes the header information for each of them.
 The header info is based upon the header in "headerfile"
 and adjusted accordingly for the cropped section.

\****************************/

static BOOL initSections(char *error)
{
	int 	hdrFile;
	BOOL	rt = TRUE;
	int		i;
	
	// Open headerfile
	if(0 > (hdrFile = open(headerfile,O_RDONLY))) {
		// open error!
		sprintf(error, "Unable to open %s for header info", headerfile);
		return(FALSE);
	}
	if(0 == readFITSHeader (hdrFile, &headinfo, error)) {
				
		for(i=0; i<numSect; i++) {
			rt &= initSection(i, &headinfo, error);			
			if(!rt) break;
		}
	}
	
	close(hdrFile);
	return(rt);
}		


// Initialize a section.
// Create a file for this section.
// Write an adjusted fits header, using the primary fits header as base
// return TRUE if successful, else FALSE. Fill error with reason.
static BOOL initSection(int section, FImage *pHead, char *error)
{
	char sectFN[1024];
	FImage fimage;
	double raDelta,decDelta;
	double raCenter,decCenter;
	double dvalue;
	double xrefpix,yrefpix,xrefdeg,yrefdeg,rot;
	char raStr[80];
	char decStr[80];
	char str[80];
	char typestr[80];
	char *type;	
	char *sfx;
	RectPtr r;
	double	dx,dy;
	double  raDeg,decDeg;
		
	// Get the CDELT amounts first -- if we can't we're pretty much outta luck
	if(0 != getRealFITS(pHead,"CDELT1",&raDelta)
	|| 0 != getRealFITS(pHead,"CDELT2",&decDelta)) {
		sprintf(error,"No CDELTn keywords");
		return FALSE;
	}
			
	// Ditto for the RA and DEC centers
	if(0 != getStringFITS(pHead,"RA",raStr)
	|| 0 != getStringFITS(pHead,"DEC",decStr)) {
		sprintf(error,"Missing RA/DEC keyword");
		return FALSE;
	}
	// and they have to be legible..
	if(0 != scansex(raStr, &raCenter)
	|| 0 != scansex(decStr, &decCenter))
	{
		sprintf(error,"Unable to extract RA/DEC values");
		return FALSE;
	}
	
	// Now get or assume CRVAL1, CRPIX1, CRVAL2, CRPIX2, CROTA2, CTYPE1
	// First, the assumptions...
	type = "-TAN";
	xrefpix = pHead->sw/2;
	yrefpix = pHead->sh/2;
	xrefdeg = hrdeg(raCenter);
	yrefdeg = decCenter;
	rot = 0.0;
	
	// now override any assumptions with any WCS solutions that might be here
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
	
	// Okay.. on with biz..
	
	// get section suffix name
	sfx = suffix[section];
	if(!sfx) {
		sprintf(str,"%02d",section);
		sfx = str;
	}
			
	sprintf(sectFN, "%s%s.fts",basename,sfx);
	if(0 > (sectList[section].fileHdl = open(sectFN, O_RDWR | O_TRUNC | O_CREAT, 0666)))
	{
		sprintf(error,"Failed to open %s",sectFN);
		return FALSE;
	}	
	
	// Create a fits header from the base information in pHead
	initFImage(&fimage);
	if(0 == copyFITSHeader(&fimage, pHead)) {
		fimage.sw = RectWidth(&sectList[section].rect);
		fimage.sh = RectHeight(&sectList[section].rect);
		setSimpleFITSHeader(&fimage);
		
		// Now adjust the RA and DEC fields
        // Get the x and y values of the center of this section
		r = &sectList[section].rect;
        dx = r->left + (RectWidth(r) / 2.0);
        dy = r->top + (RectHeight(r) / 2.0);
		/* CRPIXn assume pixels are 1-based */
		(void) worldpos(dx+1, dy+1, xrefdeg, yrefdeg, xrefpix, yrefpix, raDelta, decDelta,
					      rot, type, &raDeg, &decDeg);
					
        // write the values to the header
		fs_sexa(str,deghr(raDeg),3,360000); // hh:mm:ss.ss
		setStringFITS(&fimage,"RA",str,"Nominal center J2000 RA"); // assume no errors
		fs_sexa(str,decDeg,3,36000); // ddd:mm:ss.s
		setStringFITS(&fimage,"DEC",str,"Nominal center J2000 Dec"); // assume no errors
		
        /* Remove other RA/DEC variants that might be here.. they are no longer valid */
        delFImageVar(&fimage,"RAEOD");
        delFImageVar(&fimage,"DECEOD");
        delFImageVar(&fimage,"OBJRA");
        delFImageVar(&fimage,"OBJDEC");

        /* Remove any WCS or AMD fields from cropped images -- they must be recalculated!*/
        delWCSFITS(&fimage,0);
        /* But put back the CDELT1, 2 keywords */
        setRealFITS(&fimage,"CDELT1",raDelta,10,"RA step right, degrees/pixel");
        setRealFITS(&fimage,"CDELT2",decDelta,10,"Dec step down, degrees/pixel");

		// 2002-09-19: Add PROJCEN1 and PROJCEN2 keywords to the offset from upper left to the original pixel center
		if(!jflag) { // if we didn't specify it, default to the center
			projCen1 = (int) xrefpix - r->left;
			projCen2 = (int) yrefpix - r->top;
		}
		setIntFITS(&fimage, "PROJCEN1", projCen1, "X Coordinate of projection center");
		setIntFITS(&fimage, "PROJCEN2", projCen2,"Y Coordinate of projection center");					
		
		// Add a comment that says we've cropped this
		sprintf(str,"Mosaic cropped from file %s",basenm(imagefile));
		setCommentFITS(&fimage,"HISTORY",str);
		
		// Now write this header out
		writeFITSHeader(&fimage, sectList[section].fileHdl, error);
		
		return TRUE;
	}
	
	return FALSE;
}

/***************************\
 	streamIntoSections()
 	
 Read the image data from the named image file per row.
 If the data intersects any of our sectioned areas, we
 write them out to these files.

 Reads and writes stream simultaneously using a single row of
 the source image as a processing buffer.

\****************************/

static BOOL streamIntoSections(char * error)
{
// define a buffer chunk size used when skipping bytes to offset into image file
#define CHUNK 	1024
	char buf[CHUNK];
	int imgFile, hfile;			
	int  r,b;
	long bytesRemain;
	int pixBytes,rowBytes;
	char * pixmem;
	int i,y;
	int nb;
	char *pd;	
	Rect rowRect;
	Rect irect;
	int sw,sh,maxsh;
	
	// Open up imagefile
	imgFile = open(imagefile,O_RDONLY);
	if(imgFile < 0) return FALSE;
	
	// if imagefile is same as header file, just read past header (again)
	if(imagefile == headerfile) {
		if(0 != readFITSHeader (imgFile, &headinfo, error)) {
			close(imgFile);
			return FALSE;
		}
	}
	else // otherwise read the given byte offset
	{
		// um.. we have to read the header file separately first...
		hfile = open(headerfile,O_RDONLY);
		if(hfile < 0) {
			sprintf(error, "Error opening header file %s",headerfile);
			close(imgFile);
			return FALSE;
		}
		if(0 != readFITSHeader (hfile, &headinfo, error)) {
			sprintf(error, "Error reading header file %s",headerfile);
			close(hfile);
			close(imgFile);
			return FALSE;
		}
		close(hfile);
	
		bytesRemain = byteOffset;
		while(bytesRemain)
		{
			b = bytesRemain > CHUNK ? CHUNK : bytesRemain;
			r = read(imgFile,buf,b);
			if(r != b) {
				if(r < 0) sprintf(error,"I/O error accessing byteOffset");
				else sprintf(error,"Error skipping bytes at offset %ld of %ld",byteOffset-bytesRemain-r,byteOffset);
				close(imgFile);
				return FALSE;
			}
			bytesRemain -= r;
		}
	}
				
	// Read the imagefile data into a buffer if we are doing orientation
	if(isOrientation()) {
		streamIntoOrientation(imgFile, headinfo.sw, headinfo.sh);
		// use the width and height of the resulting orientation, not necessarily the source
		sw = getPAWidth();
		sh = getPAHeight();
		maxsh = sh > headinfo.sh ? sh : headinfo.sh;
	}
	else {
		sw = headinfo.sw;
		maxsh = sh = headinfo.sh;
	}
	// now ready to stream data and distribute
	if(headinfo.bitpix != 16) {
		fprintf(stderr,"BITPIX must be 16\n");
		exit(1);
	}
	pixBytes = headinfo.bitpix / 8;
	if(headinfo.bitpix % 8) pixBytes++;
		
	rowBytes = pixBytes * sw;
	
	// allocate a row's worth of pixel memory
	pixmem = malloc(rowBytes);
	if(pixmem) {
		
		// for y = 0 to height
		for(y=0; y<maxsh; y++) {
			// read in a row
			if(y<sh) {
				r = readImg(imgFile,pixmem,rowBytes);
			} else {
				memset(pixmem,0,rowBytes);	// fill rows beyond source but w/i section w/zeros
				r = rowBytes;
			}
			if(r != rowBytes) {
				sprintf(error,"Error reading pixel data");
				closeImg(imgFile);
				free(pixmem);
				return FALSE;
			}
	
			// Get intersection of this row with rectangles in list
			SetRect(&rowRect,0,y,sw,y+1);
			for(i=0; i<numSect; i++) {
				if(IntersectRect(&irect,&sectList[i].rect,&rowRect)) {
					// write these bytes to the file
					pd = pixmem + (irect.left * pixBytes); // offset into row
					nb = RectWidth(&irect) * pixBytes;	// bytes to write
					r = write(sectList[i].fileHdl,pd,nb);
					if(r != nb) {
						sprintf(error,"Error writing to file #%02d",i);
						closeImg(imgFile);
						free(pixmem);
						return FALSE;
					}
					// if there are additional pixels in the output section (but not the source), fill w/zeros
					nb = sectList[i].rect.right - irect.right;					
					if(nb) {
						short emptyPix = 0;
						while(nb--) write(sectList[i].fileHdl,&emptyPix,sizeof(emptyPix));
					}
				}
			}
		}
	}	
	closeImg(imgFile);
	free(pixmem);				
	return TRUE;
}

static void closeAllSections(void)
{
	int i;
	int fh;
	
	for(i=0; i<numSect; i++) {
		fh = sectList[i].fileHdl;
		if(fh >= 0) close(fh);
		sectList[i].fileHdl = -1;
		
		if(suffix[i] != NULL) {
			free(suffix[i]);
		}
	}
}

/*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*%*/

// As we find files, merge their located rectangles into the mosaic
static BOOL addComponentFITS(char *filename, char *error)
{
	int 	hdrFile;
	Rect	rect;
	double 	raDelta,decDelta;
	double 	raCenter,decCenter;
	double 	dvalue;
	double 	xrefpix,yrefpix,xrefdeg,yrefdeg,rot;
	char 	*type;
	char 	typestr[80];
	char 	raStr[80];
	char 	decStr[80];
	double 	dx,dy;
	int 	x,y;
	FImage  *pHead = &headinfo;
	
	// Open headerfile
	if(0 > (hdrFile = open(filename,O_RDONLY))) {
		// open error!
		sprintf(error, "Unable to open %s for header info", filename);
		return(FALSE);
	}
	if(0 == readFITSHeader (hdrFile, &headinfo, error)) {
		SetRect(&rect,0,0,headinfo.sw,headinfo.sh);
	
		// Get the CDELT amounts first -- if we can't we're pretty much outta luck
		if(0 != getRealFITS(pHead,"CDELT1",&raDelta)
		|| 0 != getRealFITS(pHead,"CDELT2",&decDelta)) {
			sprintf(error,"No CDELTn keywords");
			close(hdrFile);
			return FALSE;
		}
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

		// Now get or assume CRVAL1, CRPIX1, CRVAL2, CRPIX2, CROTA2, CTYPE1
		// First, the assumptions...
		type = "-TAN";
		xrefpix = pHead->sw/2;
		yrefpix = pHead->sh/2;
		xrefdeg = hrdeg(raCenter);
		yrefdeg = decCenter;
		rot = 0.0;
	
		// now override any assumptions with any WCS solutions that might be here
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
				
		// If this is the first one, it sets the basis for others
		if(!numSect) {
		
			mosaicRARefDeg = xrefdeg;
			mosaicDecRefDeg = yrefdeg;
			mosaicXRefPix = xrefpix;
			mosaicYRefPix = yrefpix;
			mosaicRADelta = raDelta;
			mosaicDecDelta = decDelta;
			mosaicRot = rot;
			strcpy(mosaicType,type);
			mosaicRect = rect;
			mosaicBitpix = pHead->bitpix;
			mosaicBinX = pHead->bx;
			mosaicBinY = pHead->by;
			// TODO? Exposure? Filter?
		}
		else
		{
		/* TODO: we need to match this! But we also need to allow
		  for some discrepancies due to solution differences.
		  How much? Should we be really fancy and scale the image to match
		  a virtual image scale?
		  In real world, on a given site, image scales should be essentially
		  equal, so I'm not sweating commenting this out right now...
		
		  DITTO for type and rotation, also
		
			if(raDelta != mosaicRADelta || decDelta != mosaicDecDelta) {
				sprintf(error,"Image scale does not match in %s",filename);
				close(hdrFile);
				return FALSE;
			}
			
			
			
		*/
			if(pHead->bitpix != mosaicBitpix) {
				sprintf(error,"Bitpix does not match in %s",filename);
				close(hdrFile);
				return FALSE;
			}
			
			if(pHead->bx != mosaicBinX || pHead->by != mosaicBinY) {
				sprintf(error,"Binning does not match in %s",filename);
				close(hdrFile);
				return FALSE;
			}
			
			// Use xypix to produce offset of image center from reference center
			xypix(xrefdeg,yrefdeg,
				  mosaicRARefDeg,mosaicDecRefDeg,
				  mosaicXRefPix,mosaicYRefPix,
				  raDelta,decDelta,
				  //rot,type,
				  //0.0,"-TAN",
				  mosaicRot,mosaicType,
				  &dx,&dy);
				
			x = (int) (dx + 0.5);
			y = (int) (dy + 0.5);
							
			// offset this to the upper-left of the image rectangle
			x -= xrefpix;
			y -= yrefpix;
			// now locate the rectangle
			OffsetRect(&rect,x,y);				
			
			// expand mosaic to include this
			MergeRect(&mosaicRect, &mosaicRect, &rect);
		}

		// record this file and its rect
		sectList[numSect].fileHdl = hdrFile;
		sectList[numSect].rect = rect;
		numSect++;
		
		return TRUE;
	}
	else
	{
		sprintf(error,"Unable to read header for %s",filename);
		close(hdrFile);
		return FALSE;
	}
}

static BOOL createMosaicFITS(char * error)
{
	FImage	fimage;
	int		pixBytes;
	int		i;
	int		nb,nr;
	int		mosaicRowBytes;
	int 	clipRowBytes;
	int		fileHdl;
	char	str[80];
	char	*mbuffer;
	char	*tbuff;
	char	*mosaddr;
	Rect	rowRect,irect;
	int		x,y;	
	double	raDeg,decDeg;
	
	// set clipping rect to be same or smaller than mosaic rect
	if(constraint.right != 0 || constraint.bottom != 0) {
		IntersectRect(&constraint,&constraint,&mosaicRect);
	} else {
		constraint = mosaicRect;
	}
	
	// create the output FITS header
	initFImage(&fimage);
	fimage.bitpix = mosaicBitpix;
	fimage.sw = RectWidth(&constraint);
	fimage.sh = RectHeight(&constraint);
	fimage.bx = mosaicBinX;
	fimage.by = mosaicBinY;
	pixBytes = mosaicBitpix / 8;
	if(mosaicBitpix % 8) pixBytes++;
	mosaicRowBytes = pixBytes * RectWidth(&mosaicRect);
	clipRowBytes = pixBytes * RectWidth(&constraint);
	setSimpleFITSHeader(&fimage);
	
	// calculate our nominal center
	x = fimage.sw / 2;
	y = fimage.sh / 2;
	/* CRPIXn assume pixels are 1-based */
	(void) worldpos(x+1, y+1, mosaicRARefDeg, mosaicDecRefDeg, mosaicXRefPix, mosaicYRefPix, mosaicRADelta, mosaicDecDelta,
				      mosaicRot, mosaicType, &raDeg, &decDeg);
	
	// Write our center
	fs_sexa(str,deghr(raDeg),3,36000); // hh:mm:ss.s
	setStringFITS(&fimage,"RA",str,"Nominal center J2000 RA"); // assume no errors
	fs_sexa(str,decDeg,3,36000); // ddd:mm:ss.s
	setStringFITS(&fimage,"DEC",str,"Nominal center J2000 Dec"); // assume no errors
	// Write our CDELT1 & CDELT2 values
	setRealFITS(&fimage,"CDELT1",mosaicRADelta,10,"RA step right, degrees/pixel");
	setRealFITS(&fimage,"CDELT2",mosaicDecDelta,10,"Dec step down, degrees/pixel");
	// Write a comment that we are a mosaic
	sprintf(str,"Mosaic created from %d files",numSect);
	setCommentFITS(&fimage,"HISTORY",str);

	// Create the file and write the header
	fileHdl = open(outfile,O_RDWR | O_TRUNC | O_CREAT, 0666);
	if(fileHdl < 0) {
		sprintf(error,"Unable to create output file %s",outfile);
		return FALSE;
	}	
	
	if(0 != writeFITSHeader(&fimage, fileHdl, error)) {
		close(fileHdl);
		return FALSE;
	}
	
	// Allocate 2 rows worth of pixel buffer (really, two buffers)
	mbuffer = malloc(mosaicRowBytes * 2);
	if(mbuffer == NULL) {
		sprintf(error,"Unable to allocate memory");
		close(fileHdl);
		return FALSE;
	}
	tbuff = mbuffer + mosaicRowBytes;
	
	// for each line
	for(y=0; y<RectHeight(&mosaicRect); y++) {
		// start with an empty (black) line
		memset(mbuffer,0,mosaicRowBytes);
		// for each section
		for(i=0; i<numSect; i++) {
			// see if we intersect
			SetRect(&rowRect,mosaicRect.left,mosaicRect.top+y,mosaicRect.right,mosaicRect.top+y+1);
			if(IntersectRect(&irect,&rowRect,&sectList[i].rect)) {
				// if so, we need to read a line from this file
				// and copy into our line buffer
				nb = RectWidth(&irect)*pixBytes;
				nr = read(sectList[i].fileHdl,tbuff,nb);
				if(nr != nb) {
					sprintf(error,"Error reading data from section %d",i);
					free(mbuffer);
					close(fileHdl);
					return FALSE;
				}
				// Copy them over..
				mosaddr = mbuffer + (sectList[i].rect.left - mosaicRect.left) * pixBytes;
				memcpy(mosaddr, tbuff, nb);					
			}
		} // do for all sections
		
		// Now write out this line
		if(IntersectRect(&irect,&rowRect,&constraint)) {
			mosaddr = mbuffer+(constraint.left-mosaicRect.left)*pixBytes;
			nb = write(fileHdl,mosaddr,clipRowBytes);
			if(nb != clipRowBytes) {
				sprintf(error,"Error writing mosaic data, line %d",y);
				free(mbuffer);
				close(fileHdl);
				return FALSE;
			}
		}
	} // do for all lines
	
	// Free our allocated buffer
	free(mbuffer);
	
	// close file -- we're done!
	close(fileHdl);
	return TRUE;			
}	

// ==========================================================================================================
/*
 * PixArrange -- orientation support
 *
 * S. Ohmert 10-11-2002
 *
*/

typedef enum
{
	none = 0,
	mirror,
	turnOver,turnOverMirror,
	turnCW,turnCWMirror,
	turnCCW,turnCCWMirror	

} ActCode;


typedef struct
{
	Rect 	srcRect;
	ActCode	action;
	int		destX;
	int		destY;
	char *	pData;
	char *  pOrigin;
	int		dxStep;
	int		dyStep;
	int 	xCount;

} Orientation;

static Orientation arrangeList[MAX_FSECT];
static int numArrange;
static Rect pixArrangeRect;
static char *pixArrangeBuffer;
static char *pBufferPix;

static int getPAWidth(void)
{
	return RectWidth(&pixArrangeRect);
}
static int getPAHeight(void)
{
	return RectHeight(&pixArrangeRect);
}
	
static void addOrientationSection(int x, int y, int width, int height, char *action, int destX, int destY)
{
	ActCode code = 0;
	// determine an ActCode from the action string
	// only allow certain literal combinations
	if(!strcasecmp(action,"none")) {
		code = none;
	} else if(!strcasecmp(action,"mirror")) {
		code = mirror;
	} else if(!strcasecmp(action,"turnCCW")) {
		code = turnCCW;
	} else if(!strcasecmp(action,"turnCW")) {
		code = turnCW;
	} else if(!strcasecmp(action,"turnOver")) {
		code = turnOver;
	} else if(!strcasecmp(action,"turnCCW+mirror")) {
		code = turnCCWMirror;
	} else if(!strcasecmp(action,"turnCW+mirror")) {
		code = turnCWMirror;
	} else if(!strcasecmp(action,"turnOver+mirror")) {
		code = turnOverMirror;
	} else {
		fprintf(stderr,"Unrecognized action '%s' -- program aborted\n",action);
		exit(1);
	}
	
	// record all the values
	SetRectWH(&arrangeList[numArrange].srcRect,x,y,width,height);
	arrangeList[numArrange].action = code;
	arrangeList[numArrange].destX = destX;
	arrangeList[numArrange].destY = destY;
	numArrange++;
}

// set up orientation buffer, if any
static BOOL initOrientation(char *error)
{
	BOOL rt = createPixArrangeBuffer(error);
	if(rt && pixArrangeBuffer) {
		arrangeIntoBuffer();
	}
	return rt;
}

// do any cleanup needed for orientation buffer
static void closeOrientation(void)
{
	if(pixArrangeBuffer) free(pixArrangeBuffer);
	pixArrangeBuffer = NULL;
	pBufferPix = NULL;
}	

// create PixArrange buffer
static BOOL createPixArrangeBuffer(char *error)
{	
	int i;
	long size;
	Orientation *pList = arrangeList;
	
	if(!numArrange) return TRUE;
	
	SetRect(&pixArrangeRect,0,0,0,0);
	for(i=0; i<numArrange; i++) {
		Rect dstRect = pList->srcRect;
		if(pList->action >= turnCW) {
			SetRectWH(&dstRect,pList->destX,pList->destY,RectHeight(&pList->srcRect),RectWidth(&pList->srcRect));
		} else {
			SetRectWH(&dstRect,pList->destX,pList->destY,RectWidth(&pList->srcRect),RectHeight(&pList->srcRect));
		}
		MergeRect(&pixArrangeRect,&pixArrangeRect,&dstRect);
		pList++;
	}
	
	size = RectWidth(&pixArrangeRect) * 2 * RectHeight(&pixArrangeRect);
	pixArrangeBuffer = (char *) malloc(size);
	if(!pixArrangeBuffer) {
		sprintf(error,"Error creating orientation buffer\n");
		return FALSE;
	}	
	pBufferPix = pixArrangeBuffer;
	return TRUE;
}

// arrange sections
static void arrangeIntoBuffer(void)
{
	int 	n;
	Orientation *pList;
	char * dp;
	int pabWidth = RectWidth(&pixArrangeRect);
//	int pabHeight = RectHeight(&pixArrangeRect);
	
	pList = arrangeList;
	n = numArrange;
	while(n--) {
		int srcW = RectWidth(&pList->srcRect);
		int srcH = RectHeight(&pList->srcRect);
		// get destination pointer into pixArrangeBuffer
		dp = pixArrangeBuffer + (pList->destY * pabWidth * 2) + (pList->destX * 2);
		switch(pList->action) {
			case none:
				pList->pOrigin = dp;
				pList->dxStep = 2;
				pList->dyStep = pabWidth * 2;
				break;
			case mirror:
				pList->pOrigin = dp + (srcW-1) * 2;
				pList->dxStep = -2;
				pList->dyStep = pabWidth*2;
				break;
			case turnCW:
				pList->pOrigin = dp + (srcH-1) * 2;
				pList->dxStep = pabWidth*2;
				pList->dyStep = -2;
				break;
			case turnCCW:
				pList->pOrigin = dp + (srcW-1) * pabWidth * 2;
				pList->dxStep = -pabWidth*2;
				pList->dyStep = 2;
				break;
			case turnOver:
				pList->pOrigin = dp + ((srcH-1) * pabWidth * 2) + (srcW * 2) - 2;
				pList->dxStep = -2;
				pList->dyStep = -pabWidth * 2;
				break;
			case turnCWMirror:
				pList->pOrigin = dp;
				pList->dxStep = pabWidth * 2;
				pList->dyStep = 2;
				break;
			case turnCCWMirror:
				pList->pOrigin = dp + ((srcW-1) * pabWidth * 2) + (srcH * 2) - 2;
				pList->dxStep = -pabWidth * 2;
				pList->dyStep = -2;
				break;
			case turnOverMirror:
				pList->pOrigin = dp + (srcH-1) * pabWidth * 2;
				pList->dxStep = 2;
				pList->dyStep = -pabWidth * 2;
				break;
		}
		pList->pData = pList->pOrigin;
		pList->xCount = RectWidth(&pList->srcRect);
		pList++;
	}		
}		
		
static BOOL isOrientation()
{
	return (pixArrangeBuffer != NULL);
}
		
// read from image file and check for
// intersections with orientation sections,
// then record into buffer accordingly
static void streamIntoOrientation(int imgFile, int w, int h)
{
	int x = 0;
	int y = 0;
	char pix[2];
	
	if(!pixArrangeBuffer) return;
	
	while(y < h) {
		if(read(imgFile,&pix,sizeof(pix)) < sizeof(pix)) {
			//error
			break;
		} else {
			// distribute to all interceptors
			int n = numArrange;
			Orientation *pList = arrangeList;
			while(n--) {						
				if(IsPointInRect(&pList->srcRect,x,y)) {
					*(pList->pData) = pix[0];
					*(pList->pData+1) = pix[1];
					pList->pData += pList->dxStep;
					pList->xCount--;
					if(!pList->xCount) {
						pList->pData = pList->pOrigin + ((y-pList->srcRect.top) * pList->dyStep);
						pList->xCount = RectWidth(&pList->srcRect);					
					}
				}
				pList++;
			}
			if(++x == w) {
				y++;
				x = 0;
			}
		}				
	}
}

// read from image file or pixArrangeBuffer, depending on context
static int readImg(int imgFile, void *buf, int len)
{
	if(!pixArrangeBuffer) {
		return read(imgFile,buf,len);
	}
	else {
		char *pBuf = (char *) buf;
		int n = len;
		while(n--) {
			*pBuf++ = *pBufferPix++;
		}
		return len;
	}
}
// close image file or destroy buffer, depending on context
static int closeImg(int imgFile)
{
	if(pixArrangeBuffer) {
		free(pixArrangeBuffer);
		pixArrangeBuffer = NULL;
		pBufferPix = NULL;
		return 0;
	}
	else {
		return close(imgFile);
	}
}

