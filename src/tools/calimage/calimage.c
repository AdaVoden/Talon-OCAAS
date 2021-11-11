/* can be used to correct raw FITS files using existing catalog corr images;
 * or can create new catalog corr images from suitable raw images.
 */

/* MODIFIED: STO20010405
   Steven Ohmert
   o	Added switch to read a bad column map file and use this to process out bad pixels.
   o	Added switch to create a bad column map file by detecting dead pixels in an image.
   o	Modified fitscorr.c also: findMapFN(), removeBadColumns()
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "P_.h"
#include "astro.h"
#include "fits.h"
#include "strops.h"
#include "fitscorr.h"
#include "telenv.h"

// STO 20021007 -- switch to computing bias by median instead of mean
#define MEDIAN_BIAS

static int correct (char *fn, char *cdn, char *bfn, char *tfn, char *ffn, char *mfn);
static int newBias (int nf, char *fns[], char *cdn, char *bfn);
static int newTherm (int nf, char *fns[], char *cdn, char *bfn, char *tfn);
static int newFlat (int nf, char *fns[], char *cdn, char *bfn, char *tfn,
    char *ffn);

/* STO20010405 */
static int newBadColMap(int nf, char *fns[], char *cdn, char *mfn);
static int fixBadColumns(FImage* fip, char *errMsg);

static void usage(char *me);
static int fitsread (char *fn, FImage *fip);
static int fitswrite (char fn[], FImage *fip);

typedef enum {
    NO_FLAG, C_FLAG, B_FLAG, T_FLAG, F_FLAG, M_FLAG, X_FLAG
} MainOpt;
static MainOpt mainopt = NO_FLAG;

static char *biasfn;
static char *thermfn;
static char *flatfn;
static char *mapfn;	/* STO20010405 */
static char *catdn;
static int filter;
static int weakVal;
static int strongVal;
static int runVal;
static int removeAll;


int
main (ac, av)
int ac;
char *av[];
{
	char *prog = av[0];
	int nbad;

	// defaults for strong/weak set to extremes. Won't likely produce any bad columns...
	weakVal = 1;
	strongVal = 65534;
	runVal = 20;
	removeAll = 0;
	
	// Read the correction config values from camera.cfg
	readCorrectionCfg(0, "archive/config/camera.cfg");	

	/* crack arguments */
	 while ((--ac > 0) && ((*++av)[0] == '-')) {
	     char *s;
	     for (s = av[0]+1; *s != '\0'; s++)
		switch (*s) {
		case 'C':
		    if (mainopt != NO_FLAG)
			usage (prog);
		    mainopt = C_FLAG;
		    break;
		case 'B':
		    if (mainopt != NO_FLAG)
			usage (prog);
		    mainopt = B_FLAG;
		    break;
		case 'T':
		    if (mainopt != NO_FLAG)
			usage (prog);
		    mainopt = T_FLAG;
		    break;
		case 'F':
		    if (mainopt != NO_FLAG)
			usage (prog);
		    mainopt = F_FLAG;
		    break;
		case 'M':  /* STO20010405 create a map file */
			if(mainopt != NO_FLAG)
			usage(prog);
			mainopt = M_FLAG;
			break;
		case 'X':  /* STO20010405 correct for map only */
			if(mainopt != NO_FLAG)
			usage(prog);
			mainopt = X_FLAG;
			break;

		case 'b':	/* set alternate bias file */
		    if (ac < 1)
			usage (prog);
		    biasfn = *(++av);
		    --ac;
		    break;
		case 't':	/* set alternate thermal file */
		    if (ac < 1)
			usage (prog);
		    thermfn = *(++av);
		    --ac;
		    break;
		case 'f':	/* set alternate flat file */
		    if (ac < 1)
			usage (prog);
		    flatfn = *(++av);
		    --ac;
		    break;
		case 'd':	/* set alternate catalog dir */
		    if (ac < 1)
			usage (prog);
		    catdn = *(++av);
		    --ac;
		    break;
		case 'l':	/* specify flat filter */
		    if (ac < 1)
			usage (prog);
		    filter = **(++av);
		    filter = toupper(filter);
		    --ac;
		    break;
		case 'm': /* STO20010405 specify alternate map file */
			if(ac < 1)
			usage (prog);
			mapfn = *(++av);
			--ac;
			break;

		case 'w': /* STO20010405 specify lower pixel range */
			if(mainopt != M_FLAG)
			usage (prog);
			weakVal = atoi(*(++av));
			--ac;
			break;
		case 's': /* STO20010405 specify upper pixel range */
			if(mainopt != M_FLAG)
			usage (prog);
			strongVal = atoi(*(++av));
			--ac;
			break;
		case 'r': /* STO20010405 specify upper pixel range */
			if(mainopt != M_FLAG)
			usage (prog);
			runVal = atoi(*(++av));
			--ac;
			break;
		case 'a': /* STO20010405 specify upper pixel range */
			if(mainopt != M_FLAG)
			usage (prog);
			removeAll = 1;
			break;


		default:
		    usage(prog);
		    break;
		}
	}

	/* require a main option */
	if (mainopt == NO_FLAG)
	    usage (prog);

	/* now there are ac args starting at av[0] -- insist on at least 1 */
	if (ac == 0)
	    usage (prog);

	/* proceed based on main option */
	nbad = 0;
	switch (mainopt) {
	case NO_FLAG:
	    usage (prog);
	    break;

	case C_FLAG:
	case X_FLAG:
	    while (ac-- > 0)
		if (correct (*av++, catdn, biasfn, thermfn, flatfn, mapfn) < 0)
		    nbad++;
	    break;

	case B_FLAG:
	    if (thermfn)
		fprintf (stderr, "Irrelevant thermal file -- ignored\n");
	    if (flatfn)
		fprintf (stderr, "Irrelevant flat file -- ignored\n");
	    if (newBias (ac, av, catdn, biasfn) < 0)
		nbad++;
	    break;

	case T_FLAG:
	    if (flatfn)
		fprintf (stderr, "Irrelevant flat file -- ignored\n");
	    if (newTherm (ac, av, catdn, biasfn, thermfn) < 0)
		nbad++;
	    break;

	case F_FLAG:
	    if (newFlat (ac, av, catdn, biasfn, thermfn, flatfn) < 0)
		nbad++;
	    break;

	case M_FLAG:
		if(newBadColMap (ac, av, catdn, mapfn) < 0)
			nbad++;
		break;
	}

	return (nbad > 0 ? 2 : 0);
}

static void
usage(me)
char *me;
{
#define	FE fprintf (stderr,

FE "%s: {-C|-B|-T|-F|-M} [options] *.fts\n", me);
FE "purpose: to apply or create image calibration correction files.\n");
FE "Bias, Thermal, and Flat files MUST exist\n");
FE " (defaults are in $TELHOME/archive/calib)\n");
FE "Bad column map file need not exist if this correction is not needed.\n");
FE "Exactly one of the following options is required:\n");
FE "  -C: fully correct each given raw file in-place\n");
FE "  -B: create a new bias corr file from the given set of raw biases\n");
FE "  -T: create a new thermal corr file from the given set of raw thermals\n");
FE "  -F: create a new flat corr file from the given set of raw flats (see -l)\n");
FE "  -M: create a new bad column map from the given set of images. See -w, -s\n");
FE "  -X: correct ONLY for bad columns.\n");
FE "The following options may also be used to specify the use of specific\n");
FE "correction files, a different catalog directory, or other features:\n");
FE "  -b fn: specify a specific bias correction file, fn\n");
FE "  -t fn: specify a specific thermal correction file, fn\n");
FE "  -f fn: specify a specific flat correction file, fn\n");
FE "  -m fn: specify a specific bad column map file, fn\n");
FE "  -d dn: specify an alternate catalog directory, dn\n");
FE "  -l f:  specify the filter, f, for -F if not in header info\n");
FE "  -w val: with -M, specifies the lower limit for a valid pixel (weak).\n");
FE "  -s val: with -M, specifies the upper limit for a valid pixel (strong).\n");
FE "  -r val: with -M, specifies the minimum run of bad pixels before recording.\n");
FE "  -a val: with -M, specifies that a run of bad pixels found will remove full column.\n");

exit (1);
}

/* correct fn */
static int
correct (char *fn, char *catdn, char *biasfn, char *thermfn, char *flatfn, char *mapfn)
{
	FImage fimage, *fip = &fimage;
	char *bn = basenm(fn);
	char catb[1024];
	char catt[1024];
	char catf[1024];
	char errmsg[1024];
	int rt;

	if (fitsread (fn, fip) < 0)
	    return (-1);

	/* gather correction images */
	if (catdn) {
	    if (!biasfn) {
		if (findBiasFN (fip, catdn, catb, errmsg) < 0) {
		    fprintf (stderr, "%s: %s\n", bn, errmsg);
		    resetFImage (fip);
		    return (-1);
		}
		biasfn = catb;
	    }
	    if (!thermfn) {
		if (findThermFN (fip, catdn, catt, errmsg) < 0) {
		    fprintf (stderr, "%s: %s\n", bn, errmsg);
		    resetFImage (fip);
		    return (-1);
		}
		thermfn = catt;
	    }
	    if (!flatfn) {
		if (findFlatFN (fip, 0, catdn, catf, errmsg) < 0) {
		    fprintf (stderr, "%s: %s\n", bn, errmsg);
		    resetFImage (fip);
		    return (-1);
		}
		flatfn = catf;
	    }
	}

	/* correct */
	if(mainopt != X_FLAG) { // STO20010405 do std correction now unless we said not to using X flag
		if (correctFITS (fip, biasfn, thermfn, flatfn, errmsg) < 0) {
			fprintf (stderr, "%s: %s\n", bn, errmsg);
			resetFImage (fip);
			return (-1);
		}
	}
	
	/* correct bad columns */
    rt = fixBadColumns(fip, errmsg);
    if(rt < 0) {
        resetFImage(fip);
		fprintf (stderr, "%s: %s\n", bn, errmsg);
        return -1;
    }

	/* save */
	return (fitswrite (fn, fip));
}

static int
fixBadColumns(FImage* fip, char *errmsg)
{
	char mapnameused[1024];
	BADCOL *badColMap;
    int rt = 0;

	/* correct bad columns */
	if(readMapFile(catdn, mapfn, &badColMap, mapnameused, errmsg) > 0) { /* will be 0 if no columns to fix */
		rt = removeBadColumns(fip, badColMap, mapnameused, errmsg);
		free(badColMap);
	}

    return rt;
}

#ifndef MEDIAN_BIAS
/* create a new net bias file from the raw biases in fns[nf].
 * new name is bfn, else put it in cdn, else put in standard place.
 */
static int
newBias (int nf, char *fns[], char *cdn, char *bfn)
{
	FImage fim, *fip = &fim;
	float *acc = NULL;
	char buf[1024];
	int iw = 0, ih = 0;
	int npix = 0;
	int i, ok;

	initFImage (fip);

	/* open and add each file */
	for (i = 0; i < nf; i++) {
	    char *fn = *fns++;
	    char *bn = basenm (fn);
	    int w, h;

	    /* read the next bias */
	    resetFImage (fip);
	    if (fitsread (fn, fip) < 0)
		break;

	    /* get size */
	    if (getNAXIS (fip, &w, &h, buf) < 0) {
			fprintf (stderr, "%s: %s\n", bn, buf);
			break;
	    }

	    /* get size and make acc if first time, based on first size */
	    if (acc == NULL) {
			iw = w;
			ih = h;
			npix = w*h;
			acc = nc_makeFPAccum(npix);
			if (!acc) {
			    fprintf (stderr, "%s: No memory\n", bn);
			    break;
			}
	    }

	    /* verify same size */
	    if (w != iw || h != ih) {
			fprintf (stderr, "%s: different size\n", bn);
			break;
	    }

	    /* accumulate into acc */
	    nc_accumulate (npix, acc, (CamPixel *)fip->image);
	}

	/* bail out now if trouble */
	ok = 0;
	if (i < nf)
	    goto out;
	   
	/* build new net bias in fip */
	nc_accdiv (npix, acc, nf);
	nc_acc2im (npix, acc, (CamPixel *)fip->image);
	nc_biaskw (fip, nf);

	/* find desired name for new bias */
	if (!bfn) {
	    char errmsg[1024];
	    if (findNewBiasFN (cdn, buf, errmsg) < 0)
			fprintf (stderr, "%s\n", errmsg);
	    else
			bfn = buf;
	}

	/* write new bias */
	if (bfn && !fitswrite (bfn, fip))
	    ok = 1;

	/* done */
    out:

	resetFImage (fip);
	if (acc)
	    free ((void *)acc);
	return (ok ? 0 : -1);
}
#else // MEDIAN_BIAS defined
/*
 * Compute bias using a MEDIAN average, not a mean.
 *
 * STO 2002-10-07
 */
static int newBias(int nf, char *fns[], char *cdn, char *bfn)
{
	FImage fim, *fip = &fim;
	CamPixel **biasList;
	char buf[1024];
	int iw = 0, ih = 0;
	int npix = 0;
	int i, n, ok;
	int mid;
		
	initFImage (fip);
			
	// Create an array of <nf> image buffers
	biasList = (CamPixel **) calloc(nf, sizeof(CamPixel *));
	if(!biasList) {
		fprintf(stderr, "Failed to allocate bias list for %d files\n",nf);
		return -1;
	}
	for(i=0; i<nf; i++) {
		biasList[i] = NULL; // start all clear
	}
	ok = 1;
	for(i=0; i<nf; i++) {
	    char *fn = *fns++;
	    char *bn = basenm (fn);
	    int w, h;

	    /* read the next bias */
	    resetFImage (fip);
	    if (fitsread (fn, fip) < 0) {
	    	ok = 0;
			break;
		}
		
	    /* get size */
	    if (getNAXIS (fip, &w, &h, buf) < 0) {
			fprintf (stderr, "%s: %s\n", bn, buf);
	    	ok = 0;
			break;
	    }

	    /* get size if first time, based on first size */
	    if (!i) {
			iw = w;
			ih = h;
	    }

	    /* verify same size */
	    if (w != iw || h != ih) {
			fprintf (stderr, "%s: different size\n", bn);
	    	ok = 0;
			break;
	    }
		
	    /* allocate room to store value */
		npix = w*h;
		biasList[i] = (CamPixel *) calloc(npix, sizeof(CamPixel));
		if(biasList[i] == NULL) {
			fprintf(stderr, "Failed to allocate memory for image %s\n",bn);
	    	ok = 0;
			break;
		}
		
		/* do insertion sort of values among bias list arrays per pixel */
		for(n=0; n<npix; n++) {
			int j,k;
			CamPixel *cp = (CamPixel *) fip->image;
			CamPixel val = cp[n];
			for(j=0; j<i; j++) {
				if((unsigned int) biasList[j][n] > (unsigned int) val)
					break;
			}
			for(k=i; k>j; k--) {
				if(k<nf) {
					biasList[k][n] = biasList[k-1][n];
				}
			}
			if(j<nf) {
				biasList[j][n] = val;
			}
		}
		
	} // next image
	
	// we now have median values arranged into the middle biasList array
	// move these values into the image
	mid = nf > 2 ? nf / 2 : 0;
	
	if(biasList[mid] != NULL) {
		memcpy(fip->image,biasList[mid],npix * sizeof(CamPixel));
	}
	// free the allocations
	for(i=0; i<nf; i++) {
		if(biasList[i]) free(biasList[i]);
	}
	
	if(ok) {	
		ok = 0;
		// set the keywords
		nc_biaskw (fip, nf);

		/* find desired name for new bias */
		if (!bfn) {
	    	char errmsg[1024];
		    if (findNewBiasFN (cdn, buf, errmsg) < 0)
				fprintf (stderr, "%s\n", errmsg);
	    	else
				bfn = buf;
		}

		/* write new bias */
		if (bfn && !fitswrite (bfn, fip)) {
		    ok = 1;
		}
	}
	
	resetFImage (fip);
	return (ok ? 0 : -1);		
}
#endif

/* create a new net thermal file from the raw thermals in fns[nf].
 * use bfn, else latest in cdn, else latest in standard place.
 * new name is tfn, else put it in cdn, else put in standard place.
 */
static int
newTherm (int nf, char *fns[], char *cdn, char *bfn, char *tfn)
{
	FImage fim, *fip = &fim;
	float *acc = NULL;
	char errmsg[1024];
	char biasfn[1024];
	char thermfn[1024];
	int iw = 0, ih = 0;
	int npix = 0;
	int i, ok;

	initFImage (fip);

	/* open and add each file */
	for (i = 0; i < nf; i++) {
	    char *fn = *fns++;
	    char *bn = basenm (fn);
	    int w, h;

	    /* read the next thermal */
	    resetFImage (fip);
	    if (fitsread (fn, fip) < 0)
		break;

	    /* get size */
	    if (getNAXIS (fip, &w, &h, errmsg) < 0) {
		fprintf (stderr, "%s: %s\n", bn, errmsg);
		break;
	    }

	    /* get size and make acc if first time, based on first size */
	    if (acc == NULL) {
		iw = w;
		ih = h;
		npix = w*h;
		acc = nc_makeFPAccum(npix);
		if (!acc) {
		    fprintf (stderr, "%s: No memory\n", bn);
		    break;
		}
	    }

	    /* verify same size */
	    if (w != iw || h != ih) {
		fprintf (stderr, "%s: different size\n", bn);
		break;
	    }

	    /* accumulate into acc */
	    nc_accumulate (npix, acc, (CamPixel *)fip->image);
	}

	/* bail out now if trouble */
	ok = 0;
	if (i < nf)
	    goto out;

	/* find average thermal */
	nc_accdiv (npix, acc, nf);

	/* find desired bias name */
	if (!bfn) {
	    if (findBiasFN (fip, cdn, biasfn, errmsg) < 0)
		fprintf (stderr, "%s: %s\n", cdn?cdn:"<default>", errmsg);
	    else
		bfn = biasfn;
	}
	if (bfn) {
	    /* subtract bias */
	    if (nc_applyBias (npix, acc, bfn, errmsg) < 0) {
		fprintf (stderr, "%s: %s\n", basenm(bfn), errmsg);
		goto out;
	    }
	    nc_acc2im (npix, acc, (CamPixel *)fip->image);
	    nc_thermalkw (fip, nf, bfn);

	    /* find desired name for new thermal */
	    if (!tfn) {
		if (findNewThermFN (cdn, thermfn, errmsg) < 0)
		    fprintf (stderr, "%s: %s\n", cdn?cdn:"<default>", errmsg);
		else
		    tfn = thermfn;
	    }

	    /* write new thermal */
	    if (tfn && !fitswrite (tfn, fip))
		ok = 1;
	}

	/* done */
    out:
	resetFImage (fip);
	if (acc)
	    free ((void *)acc);
	return (ok ? 0 : -1);
}

/* create a new net flat file from the raw flats in fns[nf].
 * use bfn, else latest in cdn, else latest in standard place.
 * use tfn, else latest in cdn, else latest in standard place.
 * new name is ffn, else put it in cdn, else put in standard place.
 */
static int
newFlat (int nf, char *fns[], char *cdn, char *bfn, char *tfn, char *ffn)
{
	FImage fim, *fip = &fim;
	float *acc = NULL;
	char errmsg[1024];
	char biasfn[1024];
	char thermfn[1024];
	char flatfn[1024];
	double dur = 0;
	int iw = 0, ih = 0;
	int npix = 0;
	int i, ok;

	initFImage (fip);

	/* open and add each file */
	for (i = 0; i < nf; i++) {
	    char *fn = *fns++;
	    char *bn = basenm (fn);
	    double thisdur;
	    int w, h;

	    /* read the next flat */
	    resetFImage (fip);
	    if (fitsread (fn, fip) < 0)
		break;

	    /* get size */
	    if (getNAXIS (fip, &w, &h, errmsg) < 0) {
		fprintf (stderr, "%s: %s\n", bn, errmsg);
		break;
	    }

	    /* get size and make acc if first time, based on first size */
	    if (acc == NULL) {
		iw = w;
		ih = h;
		npix = w*h;
		acc = nc_makeFPAccum(npix);
		if (!acc) {
		    fprintf (stderr, "%s: No memory\n", bn);
		    break;
		}
	    }

	    /* verify same size */
	    if (w != iw || h != ih) {
		fprintf (stderr, "%s: different size\n", bn);
		break;
	    }
	    
	    /* verify same (or no) filter */
	    if (getStringFITS (fip, "FILTER", errmsg) == 0) {
		int f = toupper(errmsg[0]);
		if (!filter)
		    filter = f;
		else if (f != filter) {
		    fprintf (stderr, "%s: filter mismatch\n", bn);
		    break;
		}
	    } else if (!filter) {
		fprintf (stderr,"%s: unspecified filter -- use -l option\n",bn);
		break;
	    }

	    /* accumulate exp time */
	    if (getRealFITS (fip, "EXPTIME", &thisdur) < 0) {
		fprintf (stderr, "%s: no EXPTIME\n", bn);
		break;
	    }
	    dur += thisdur;

	    /* accumulate into acc */
	    nc_accumulate (npix, acc, (CamPixel *)fip->image);
	}

	/* bail out now if trouble */
	ok = 0;
	if (i < nf)
	    goto out;

	/* find average flat and its effective duration */
	nc_accdiv (npix, acc, nf);
	dur /= nf;
	    
	/* find desired bias name */
	if (!bfn) {
	    if (findBiasFN (fip, cdn, biasfn, errmsg) < 0)
		fprintf (stderr, "%s: %s\n", cdn?cdn:"<default>", errmsg);
	    else
		bfn = biasfn;
	}
	if (bfn) {
	    /* subtract bias */
	    if (nc_applyBias (npix, acc, bfn, errmsg) < 0) {
		fprintf (stderr, "%s: %s\n", basenm(bfn), errmsg);
		goto out;
	    }

	    /* find desired thermal */
	    if (!tfn) {
		if (findThermFN (fip, cdn, thermfn, errmsg) < 0)
		    fprintf (stderr, "%s: %s\n", cdn?cdn:"<default>", errmsg);
		else
		    tfn = thermfn;
	    }
	    if (tfn) {
		/* subtract thermal */
		if (nc_applyThermal (npix, acc, dur, tfn, errmsg) < 0) {
		    fprintf (stderr, "%s: %s\n", basenm(tfn), errmsg);
		    goto out;
		}
		nc_acc2im (npix, acc, (CamPixel *)fip->image);
		nc_flatkw (fip, nf, bfn, tfn, filter);

		/* find desired name for new flat */
		if (!ffn) {
		    if (findNewFlatFN (filter, cdn, flatfn, errmsg) < 0)
			fprintf(stderr, "%s: %s\n", cdn?cdn:"<default>",errmsg);
		    else
			ffn = flatfn;
		}

		/* write new flat */
		if (ffn && !fitswrite (ffn, fip))
		    ok = 1;
	    }
	}

	/* done */
    out:
	resetFImage (fip);
	if (acc)
	    free ((void *)acc);
	return (ok ? 0 : -1);
}

/* 
 * STO20010405
 * create a new bad column map by detecting runs of 
 * overly bright or overly dark pixels in reference image
 */

#define DARKLIMIT	weakVal
#define BRIGHTLIMIT	strongVal
#define RUNLIMIT	runVal

static int newBadColMap(int nf, char *fns[], char *cdn, char *mfn)
{
	FImage fim, *fip = &fim;
	float *acc = NULL;
	char buf[1024];
	int iw = 0, ih = 0;
	int npix = 0;
	int i, ok;
	FILE * fp;
	char *firstFile = fns[0];

	initFImage (fip);

	/* open and add each file */
	for (i = 0; i < nf; i++) {
	    char *fn = *fns++;
	    char *bn = basenm (fn);
	    int w, h;

	    /* read the next bias */
	    resetFImage (fip);
	    if (fitsread (fn, fip) < 0)
		break;

	    /* get size */
	    if (getNAXIS (fip, &w, &h, buf) < 0) {
		fprintf (stderr, "%s: %s\n", bn, buf);
		break;
	    }

	    /* get size and make acc if first time, based on first size */
	    if (acc == NULL) {
		iw = w;
		ih = h;
		npix = w*h;
		acc = nc_makeFPAccum(npix);
		if (!acc) {
		    fprintf (stderr, "%s: No memory\n", bn);
		    break;
		}
	    }

	    /* verify same size */
	    if (w != iw || h != ih) {
		fprintf (stderr, "%s: different size\n", bn);
		break;
	    }

	    /* accumulate into acc */
	    nc_accumulate (npix, acc, (CamPixel *)fip->image);
	}

	/* bail out now if trouble */
	ok = 0;
	if (i < nf)
	    goto out;

	/* take a mean of all the constituent images to get averaged pixel values */
	nc_accdiv(npix, acc, nf);

	/* open a new map file */
	if (!mfn) {
	    char errmsg[1024];
	    if (findNewMapFN (cdn, buf, errmsg) < 0)
		fprintf (stderr, "%s\n", errmsg);
	    else
		mfn = buf;
	}
	fp = fopen(mfn, "w+");

	/* Write commentary header info */
	fprintf(fp, "! Bad Column Mapping\n");
	fprintf(fp, "! Generated by calimage -M -w %d -s %d\n",weakVal, strongVal);
	if(nf == 1) {
		fprintf(fp, "! using %d x %d FITS image '%s' as source\n", fip->sw, fip->sh, firstFile);
	} else {
		fprintf(fp, "! using %d combined %d x %d FITS images as source\n", nf, fip->sw, fip->sh);
	}
	fprintf(fp, "!\n");
	fprintf(fp, "! This file may be hand-edited if desired.\n");
	fprintf(fp, "! Each line in this file depicts a range of pixels which are considered 'bad'\n");
	fprintf(fp, "! and are to be replaced with the mean average of the adjacent pixels.\n");
	fprintf(fp, "! Each entry has the following format:\n");
	fprintf(fp, "! <column>  <begin>  <end>\n");
	fprintf(fp, "! where <column> is the pixel column (left is zero)\n");
	fprintf(fp, "! and where <begin> and <end> are the start and ending values, inclusive,\n");
	fprintf(fp, "! for the rows in this column (top is zero) to be removed.\n");
	fprintf(fp, "\n");

	/* Scan the map looking for hot/dead pixels and recording as necessary */
	if(fp != NULL)
	{
		int column,row;
		int begin,end;
		float * base = acc;
		float * p;

		for(column = 0; column < iw; column++) {
			row = 0;
			p = base + column;
			while(row < ih) {
				if(*p > BRIGHTLIMIT || *p < DARKLIMIT) {
					begin = row;
					while(*p > BRIGHTLIMIT || *p < DARKLIMIT) {
						row++;
						p += iw;
						if(row >= ih) break;
					}
					end = row-1;
					/* record column, begin, end */
					/* runs must be more than RUNLIMIT long */
					if(end-begin > RUNLIMIT)
					{
						if(removeAll) {
							begin = 0;
							end = 9999;
						}
						/*DB fprintf(fp, "%4d %4d %4d\n",column,begin,end); */
					}
				}
				else
				{
					p+=iw;
					row++;
				}
			}
		}

		fclose(fp);
		ok = 1;
	}
	else
	{
		fprintf(stderr, "unable to open %s for writing\n",mfn);
	}

	/* done */
    out:
	resetFImage (fip);
	if (acc)
	    free ((void *)acc);
	return (ok ? 0 : -1);
}


/* open fn, read into fip, close fn.
 * return 0 if ok, else -1.
 */
static int
fitsread (char *fn, FImage *fip)
{
	char *bn = basenm(fn);
	char errmsg[1024];
	int s, fd;
	    
	/* open */
	fd = open (fn, O_RDWR);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", bn, strerror(errno));
	    return (-1);
	}

	/* read */
	initFImage (fip);
	s = readFITS (fd, fip, errmsg);
	(void) close (fd);
	if (s < 0) {
	    fprintf (stderr, "%s: %s\n", bn, errmsg);
	    return (-1);
	}

	/* ok */
	return (0);
}

/* (over)write fn with fip.
 * return 0 if ok else -1.
 */
static int
fitswrite (char fn[], FImage *fip)
{
	char *bn = basenm (fn);
	char errmsg[1024];
	int s, fd;

	fd = open (fn, O_RDWR | O_TRUNC | O_CREAT, 0666);
	if (fd < 0) {
	    fprintf (stderr, "%s: %s\n", bn, strerror(errno));
	    resetFImage (fip);
	    return (-1);
	}
	s = writeFITS (fd, fip, errmsg, 0);
	(void) close (fd);
	resetFImage (fip);
	if (s < 0) {
	    fprintf (stderr, "%s: %s\n", bn, errmsg);
	    return (-1);
	}

	/* ok */
	return (0);
}


/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $Id: calimage.c,v 1.1.1.1 2003/04/15 20:48:32 lostairman Exp $"};
