/***************************************************************************
 * findobj.c  - identifies elongated objects in an image.
 *                            -------------------
 *
 * New version -- replaces former work by John Armstrong
 * Utilizes new streak detection functions of fitsip.c (libfits).
 *
 * $Id: findobj.c,v 1.1.1.1 2003/04/15 20:48:33 lostairman Exp $
 * $Log: findobj.c,v $
 * Revision 1.1.1.1  2003/04/15 20:48:33  lostairman
 * Initial Import
 *
 * Revision 1.7  2002/12/28 20:09:36  steve
 * made ip.cfg location settable by caller
 *
 * Revision 1.6  2002/11/25 19:53:37  steve
 * updated for new streak detection library code
 *
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "fits.h"
#include "strops.h"
#include "wcs.h"

static void usage (char *p);
static void findem (char *fn);

static char *progname;
static int rflag;
static int hflag;
static int sflag;
static int lflag;
static int aflag;
static int pflag;
static int dflag;
static int xflag;

int
main (int ac, char *av[])
{
  char *str;
  char oldIpCfg[256];

  // Save existing IP.CFG.... be sure to restore before any exit!!	
  strcpy(oldIpCfg,getCurrentIpCfgPath());

  /* crack arguments */
  progname = basenm(av[0]);
  for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
    char c;
    while ((c = *++str)) {
      switch (c) {
      case 'r':
		rflag++;
		break;
      case 'h':
		hflag++;
		break;
      case 's':
		sflag++;
		break;
      case 'l':
		lflag++;
		break;
      case 'a':
		lflag++;
		aflag++;
		break;
	  case 'p':
		pflag++;
		break;
	  case 'd':
		dflag++;
		break;
	  case 'x':
		xflag++;
		break;
		case 'i':	/* alternate ip.cfg */
			if(ac < 2) {
				fprintf(stderr, "-i requires ip.cfg pathname\n");
				setIpCfgPath(oldIpCfg);
				usage(progname);
			}
			setIpCfgPath(*++av);
			ac--;
			break;
		default:
			setIpCfgPath(oldIpCfg);
		    usage (progname);
		    break;
      }
    }
  }
  
  if (ac == 0) {
	setIpCfgPath(oldIpCfg);
    usage (progname);
  }

  // defaults to -sx (standard findstars)
  if(!(pflag|dflag|rflag|hflag|xflag)) xflag++;
  if(!(sflag|lflag)) sflag++;

  while (ac-- > 0)
    findem (*av++);
  
  setIpCfgPath(oldIpCfg);
  return (0);
}

static void
usage (char *p)
{
	fprintf (stderr, "Usage: %s [options] *.fts\n", p);
	fprintf (stderr, "Purpose: list objects in FITS files\n");
	fprintf (stderr, "Options:\n");
	fprintf (stderr, " -s: find stars (default), choose output format with flags -r,-h,-p below\n");
	fprintf (stderr, " -l: find linear features, choose output format with flags -r,-p below\n");
	fprintf (stderr, " -a: report on all objects (stars and streak) found with streak detection\n");
	fprintf (stderr, " -p: report in pixel coordinates\n");
	fprintf (stderr, " -r: report WCS data in rads\n");
	fprintf (stderr, " -d: report WCS data in degrees\n");
	fprintf (stderr, " -x: report WCS data HH:MM:SS, DD:MM:SS format\n");
	fprintf (stderr, " -h: high prec format: Alt Az RA Dec M HW VW JD dt\n");
	fprintf (stderr, " -i path: specify alternate ip.cfg location\n");

	exit (1);
}

static void
findem (char *fn)
{
  FImage fimage, *fip = &fimage;
  Now now, *np = &now;
  double eq = 0;
  double lst = 0;
  double expt = 0;
  StreakData *streakList;
  char buf[1024];
  StarStats *ssp;
  int i,ns=0,nl=0,fd;
  
  // print header with filename //
  printf("# %s\n", fn);
  /* open and read the image */
  fd = open (fn, O_RDONLY);
  if (fd < 0) {
    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
    return;
  }
  initFImage (fip);
  i = readFITS (fd, fip, buf);
  (void) close (fd);
  if (i < 0) {
    fprintf (stderr, "%s: %s\n", fn, buf);
    resetFImage (fip);
    return;
  }
  
  if (sflag) {
    /* gather local info if want alt/az */
    if (hflag) {
      double tmp;
      
      memset (np, 0, sizeof(Now));
      
      if (getRealFITS (fip, "JD", &tmp) < 0) {
	fprintf (stderr, "%s: no JD\n", fn);
	resetFImage (fip);
	return;
      }
      mjd = tmp - MJD0;
      
      if (getStringFITS (fip, "LATITUDE", buf) < 0) {
	fprintf (stderr, "%s: no LATITUDE\n", fn);
	return;
      }
      if (scansex (buf, &tmp) < 0) {
	fprintf (stderr, "%s: badly formed LATITUDE: %s\n", fn, buf);
	return;
      }
      lat = degrad(tmp);
      
      if (getStringFITS (fip, "LONGITUD", buf) < 0) {
	fprintf (stderr, "%s: no LONGITUD\n", fn);
	return;
      }
      if (scansex (buf, &tmp) < 0) {
	fprintf (stderr, "%s: badly formed LONGITUD: %s\n", fn, buf);
	return;
      }
      lng = degrad(tmp);
      
      temp = getStringFITS (fip, "WXTEMP", buf) ? 10 : atof(buf);
      pressure = getStringFITS (fip, "WXPRES", buf) ? 1010 : atof(buf);
      
      elev = 0;	/* ?? */
      
      if (getRealFITS (fip, "EQUINOX", &eq) < 0
	  && getRealFITS (fip, "EPOCH", &eq) < 0) {
	fprintf (stderr, "%s: no EQUINOX or EPOCH\n", fn);
	resetFImage (fip);
	return;
      }
      year_mjd (eq, &eq);
      
      if (getStringFITS (fip, "LST", buf) < 0) {
	fprintf (stderr, "%s: no LST\n", fn);
	return;
      }
      if (scansex (buf, &tmp) < 0) {
	fprintf (stderr, "%s: badly formed LST: %s\n", fn, buf);
	return;
      }
      lst = hrrad(tmp);
      
      if (getRealFITS (fip, "EXPTIME", &expt) < 0) {
	fprintf (stderr, "%s: no EXPTIME\n", fn);
	resetFImage (fip);
	return;
      }
    }
  }
  
  // search for linear features //

  if (lflag) {	
    double ra1, dec1;
    double ra2, dec2;
    char rastr1[32], decstr1[32];
    char rastr2[32], decstr2[32];
    int st,maybe;
	static int saiderr;
	
    saiderr = 0;
    (void) findStarsAndStreaks(fip->image,fip->sw,fip->sh,NULL,NULL,NULL,&streakList,&nl);
	st = maybe = 0;
    for(i=0; i<nl; i++) {
    	if((streakList[i].flags & STREAK_MASK)) {
    		st++;
    		if((streakList[i].flags & STREAK_MASK) == STREAK_MAYBE) {
    			maybe++;
    		}
    	}
    }
    if(aflag) {
    	printf("# %d object%s detected\n", nl, nl==1 ? "" : "s");
    }
    printf("# %d object%s found to be streak-like\n",st, st==1 ? "" : "s");
    if(maybe) {
	    printf("# %d considered likely to be%s true streak%s\n",st-maybe, st-maybe==1?" a":"", st-maybe==1?"":"s");
    	printf("# %d considered as%s possible streak%s\n",maybe, maybe==1?" a":"", maybe==1?"":"s");
    }
    for (i=0;i<nl;i++) {
    	if(aflag || (streakList[i].flags&STREAK_MASK) ) {
	    	if (xy2RADec (fip, streakList[i].startX,streakList[i].startY, &ra1, &dec1) < 0) {
				if(!saiderr++) fprintf (stderr, "%s: no WCS\n", fn);
			}
			else {
				xy2RADec (fip, streakList[i].endX,streakList[i].endY, &ra2, &dec2);
			}
			if(pflag) {
				printf("%4d,%4d (to %4d,%4d) slope: %2.4lf length: %3d walkStart: %4d,%4d walkEnd: %4d,%4d fwhmRatio: %2.4lf flags: %04X",
					streakList[i].startX,streakList[i].startY, streakList[i].endX,streakList[i].endY,
					streakList[i].slope,streakList[i].length,
					streakList[i].walkStartX,streakList[i].walkStartY,
					streakList[i].walkEndX,streakList[i].walkEndY,
					streakList[i].fwhmRatio, streakList[i].flags);
			}
			if(!saiderr) {
				if(rflag) {
					printf("% 9.6lf %9.6lf  % 9.6lf %9.6lf ", ra1,dec1,ra2,dec2);
				}
				else if(dflag) {
					printf("% 9.6lf %9.6lf  % 9.6lf %9.6lf ", raddeg(ra1),raddeg(dec1), raddeg(ra2),raddeg(dec2));
				}
				else if(xflag) {
					fs_sexa (rastr1, radhr (ra1), 2, 360000);
					fs_sexa (decstr1, raddeg (dec1), 3, 36000);
					fs_sexa (rastr2, radhr (ra2), 2, 360000);
					fs_sexa (decstr2, raddeg (dec2), 3, 36000);
					printf ("%s %s %s %s ", rastr1, decstr1, rastr2, decstr2);
				}
				else if(hflag) {
					printf("hi-prec format not supported for -l option");
				}
			   printf("\n");
		   }
        } // end aflag || streak
	} // end for
  } // end lflag

  	
  // start the standard findstars algorithm //
  
  if (sflag) {
    ns = findStatStars (fip->image, fip->sw, fip->sh, &ssp);
    printf("# %d Stars Found\n", ns);
    for (i = 0; i < ns; i++) {
      StarStats *sp = &ssp[i];
      double ra, dec;
      
      if (xy2RADec (fip, sp->x, sp->y, &ra, &dec) < 0) {
	fprintf (stderr, "%s: no WCS\n", fn);
	break;
      }

      if(pflag) {
		printf("%4d, %4d ",(int) (sp->x+.5),(int) (sp->y+.5));
      }

      if (rflag)
	printf ("%9.6f %9.6f %5d\n", ra, dec, sp->p);
      else if (hflag) {
	double mag, dmag;
	double apra, apdec;
	double alt, az;
	double ha;
	
	apra = ra;
	apdec = dec;
	as_ap (np, eq, &apra, &apdec);
	ha = lst - apra;
	hadec_aa (lat, ha, apdec, &alt, &az);
	refract (pressure, temp, alt, &alt);
	
	starMag (&ssp[0], sp, &mag, &dmag);
//	printf ("%d %g %d %g\n", ssp[0].Src, ssp[0].rmsSrc, sp->Src, sp->rmsSrc);
	printf ("%9.6f %9.6f  %9.6f %9.6f  %5.2f  %5.2f %5.2f  %14.6f %6.1f\n",
		raddeg(alt), raddeg(az), raddeg(ra), raddeg(dec),
		mag, sp->xfwhm, sp->yfwhm, mjd+MJD0-expt/SPD, expt);
      } else if(xflag) {
	char rastr[32], decstr[32];
	fs_sexa (rastr, radhr (ra), 2, 36000);
	fs_sexa (decstr, raddeg (dec), 3, 3600);
	printf ("%s %s %5d\n", rastr, decstr, ssp[i].p);
	
      } else {
      	printf("\n");
      }
    }
    
    if (ns >= 0)
      free ((void *)ssp);
    resetFImage (fip);
  }
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: findobj.c,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $"};
