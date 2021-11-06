/* this is a simple command-line program to operate the generic CCD camera
 * interface. Each image successfully taken is stored in minimal FITS format
 * and assigned the name image<n>.fts, where n starts at 0 and increases by
 * one each time. The path to the device driver may be specified on the
 * command line, else a default is used. Also, the name of each new files is
 * sent to the fifo CameraFilename to inform the Camera to display it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <time.h>

#include "fits.h"
#include "telenv.h"
#include "ccdcamera.h"

static char path_def[] = "dev/ccdcamera";

static CCDExpoParams cep;		/* persistent exposure info */
static int clipfile;			/* set to clip resulting file */
static int filex, filey, filew, fileh;	/*   if clipfile, defines region */

static void usage (char *me);
static void help (void);
static void writeFits (CCDExpoParams *cep, char *pix);
static int simpleFITS (FILE *fp, char *pix, CCDExpoParams *cp);
static int simpleClippedFITS (FILE *fp, char *pix, CCDExpoParams *cp);
static void fitsHeader (FILE *fp, int x, int y, int w, int h, int bx,
    int by, int duration);
static void pad_2880(FILE *fp);

int
main (int ac, char *av[])
{
	CCDExpoParams tmpcep;
	CCDTempInfo tinfo;
	struct timeval tv, tv2;
	double dt;
	char buf[1024];
	char *imagebuf;
	int nbytes;
	fd_set rfd;
	int camfd;
	char path[1024];
	int n;

	/* decide path to driver */
	if (ac == 1)
	    strcpy (path, path_def);
	else if (av[1][0] == '-')
	    usage(av[0]);
	else
	    strcpy (path, av[1]);
	telfixpath (path, path);

	/* open the driver */
	if (setPathCCD (path, 0, buf) < 0) {
	    printf ("%s: %s\n", path, buf);
	    exit(1);
	}

	/* print the ID string */
	if (getIDCCD (buf, buf) < 0) {
	    printf ("Can not get ID: %s\n", buf);
	    exit(1);
	}
	printf ("ID: %s\n", buf);


	/* decide default exposure info */
	if (getSizeCCD (&cep, buf) < 0) {
	    printf ("Can not get chip info: %s\n", buf);
	    exit(1);
	}
	cep.bx = cep.by = 1;
	cep.sx = cep.sy = 0;
	cep.duration = 1000;
	cep.shutter = CCDSO_Open;
	if (setExpCCD (&cep, buf) < 0) {
	    printf ("Setup error: %s\n", buf);
	    exit (1);
	}

	/* malloc max image buffer */
	nbytes = cep.sw * cep.sh * sizeof(CamPixel);
	imagebuf = malloc (nbytes);
	if (!imagebuf) {
	    printf ("No memory for %dx%d array\n", cep.sw, cep.sh);
	    exit(1);
	}
	printf ("Max size: %d W x %d H\n", cep.sw, cep.sh);

	while (printf ("> "), fgets (buf, sizeof(buf), stdin)) {
	    switch (buf[0]) {

	    case '0':   /* close shutter */
		if (setShutterNow (0, buf) < 0)
		    printf ("%s\n", buf);
		break;

	    case '1':   /* open shutter */
		if (setShutterNow (1, buf) < 0)
		    printf ("%s\n", buf);
		break;

	    case 'b':	/* set binning */
		clipfile = 0;
		if (sscanf (buf, "%*c %d %d", &cep.bx, &cep.by) != 2)
		    printf ("Bad binning format\n");
		else if (setExpCCD (&cep, buf) < 0)
		    printf ("%s\n", buf);
		else
		    printf ("Ok\n");
		break;

	    case 'C':	/* set cooler */
		tinfo.s = CCDTS_SET;
		if (sscanf (buf, "%*c %d", &tinfo.t) != 1)
		    printf ("Bad temp format\n");
		else if (setTempCCD (&tinfo, buf) < 0)
		    printf ("%s\n", buf);
		else
		    printf ("Setting temp to %d\n", tinfo.t);
		break;

	    case 'c':	/* turn cooler off */
		tinfo.s = CCDTS_OFF;
		if (setTempCCD (&tinfo, buf) < 0)
		    printf ("%s\n", buf);
		else
		    printf ("Cooler off\n");
		break;

	    case 'd':	/* set duration */
		if (sscanf (buf, "%*c %d", &cep.duration) != 1)
		    printf ("Bad duration format\n");
		else if (setExpCCD (&cep, buf) < 0)
		    printf ("%s\n", buf);
		else
		    printf ("Ok\n");
		break;

	    case 'f':	/* set full-frame */
		if (getSizeCCD (&tmpcep, buf) < 0)
		    printf ("Can not get chip info: %s\n", buf);
		else {
		    cep.sw = tmpcep.sw;
		    cep.sh = tmpcep.sh;
		    cep.sx = 0;
		    cep.sy = 0;
		    cep.bx = 1;
		    cep.by = 1;
		    if (setExpCCD (&cep, buf) < 0) {
			printf ("%s\n", buf);
			break;
		    }

		    /* look for additional file size info.
		     * if find, set filex/y/w/h and set clipfile.
		     */
		    clipfile = 0;
		    n = sscanf (buf, "%*c %d %d %d %d",
						&filex, &filey, &filew, &fileh);
		    switch (n) {
		    case 4:	/* found 4 params, check against chip */
			if (filex < 0 || filex + filew > cep.sw
					|| filey < 0 || filey + fileh > cep.sh)
			    printf ("File size not wholly within chip.\n");
			else
			    clipfile = 1;	/* yes! */
			break;
		    case -1:	/* no attempt */
		    case 0:
			break;
		    default:	/* partial attempt */
			printf ("Bogus file size.\n");
			break;
		    }
		}
		break;

	    case 'h':	/* control shutter mode */
		switch (buf[2]) {
		case 'o':
		    cep.shutter = CCDSO_Open;
		    printf ("shutter will be opened\n");
		    break;
		case 'c':
		    cep.shutter = CCDSO_Closed;
		    printf ("shutter will remain closed\n");
		    break;
		case 'd':
		    cep.shutter = CCDSO_Dbl;
		    printf ("take a exposure in pattern OOCO\n");
		    break;
		case 'm':
		    cep.shutter = CCDSO_Multi;
		    printf ("take a exposure in pattern OOCOCO\n");
		    break;
		default:
		    printf ("use o/c/d/m\n");
		    continue;
		}
		if (setExpCCD (&cep, buf) < 0)
		    printf ("%s\n", buf);
		break;

	    case 'm':	/* report temp */
		if (getTempCCD (&tinfo, buf) < 0)
		    printf ("%s\n", buf);
		else {
		    buf[0] = '\0';
		    switch (tinfo.s) {
		    case CCDTS_AT:      strcat (buf, "AtTarg"); break;
		    case CCDTS_UNDER:   strcat (buf, "<Targ"); break;
		    case CCDTS_OVER:    strcat (buf, ">Targ"); break;
		    case CCDTS_OFF:     strcat (buf, "Off"); break;
		    case CCDTS_RDN:     strcat (buf, "Ramping"); break;
		    case CCDTS_RUP:     strcat (buf, "ToAmb"); break;
		    case CCDTS_STUCK:   strcat (buf, "Floor"); break;
		    case CCDTS_MAX:     strcat (buf, "Ceiling"); break;
		    case CCDTS_AMB:     strcat (buf, "AtAmb"); break;
		    default:            strcat (buf, "Error"); break;
		    }
		    if (tinfo.s != CCDTS_OFF)
			printf ("%d C, ", tinfo.t);
		    printf ("%s\n", buf);
		}
		break;

	    case 'q':	/* report chip size */
		if (getSizeCCD (&tmpcep, buf) < 0)
		    printf ("%s\n", buf);
		else {
		    printf ("Max size:    %4d W x %4d H\n",tmpcep.sw,tmpcep.sh);
		    printf ("Max binning: %4d H x %4d V\n",tmpcep.bx,tmpcep.by);
		}
		break;

	    case 'R':	/* read */
		/* start exposure if not already in progress */
		if (selectHandleCCD (buf) < 0) {
		    if (startExpCCD (buf) < 0) {
			printf ("%s\n", buf);
			break;
		    }
		}
		nbytes = (cep.sw/cep.bx) * (cep.sh/cep.by) * sizeof(CamPixel);
		gettimeofday (&tv, NULL);
		if (readPixelCCD (imagebuf, nbytes, buf) < 0) {
		    printf ("%s\n", buf);
		    break;
		}
		gettimeofday (&tv2, NULL);

		writeFits (&cep, imagebuf);
		dt = tv2.tv_sec - tv.tv_sec + (tv2.tv_usec - tv.tv_usec)/1e6
					    - cep.duration/1e3;
		printf ("%d pixels in %g secs <=> %g pix/sec\n",
						    nbytes/2, dt, nbytes/2/dt);
		break;

	    case 'r':	/* non-blocking read */
		/* start exposure if not already in progress */
		if (selectHandleCCD (buf) < 0) {
		    if (startExpCCD (buf) < 0) {
			printf ("%s\n", buf);
			break;
		    }
		}
		nbytes = (cep.sw/cep.bx) * (cep.sh/cep.by) * sizeof(CamPixel);
		gettimeofday (&tv, NULL);
		if (readPixelNBCCD (imagebuf, nbytes, buf) < 0) {
		    printf ("%s\n", buf);
		    break;
		}
		gettimeofday (&tv2, NULL);

		writeFits (&cep, imagebuf);
		dt = tv2.tv_sec - tv.tv_sec + (tv2.tv_usec - tv.tv_usec)/1e6
					    - cep.duration/1e3;
		printf ("%d pixels in %g secs <=> %g pix/sec\n",
						    nbytes/2, dt, nbytes/2/dt);
		break;

	    case 'S':   /* wait for pixels ready */
		camfd = selectHandleCCD (buf);
		if (camfd < 0) {
		    printf ("%s\n", buf);
		    break;
		}
		FD_ZERO (&rfd);
		FD_SET (camfd, &rfd);
		n = select (camfd+1, &rfd, NULL, NULL, NULL);
		if (n < 0)
		    perror ("S");
		else if (FD_ISSET (camfd, &rfd))
		    printf ("camfd is ready for reading\n");
		else
		    printf ("Nothing ready\n");
		break;

	    case 's':   /* poll for pixels ready */
		camfd = selectHandleCCD (buf);
		if (camfd < 0) {
		    printf ("%s\n", buf);
		    break;
		}
		FD_ZERO (&rfd);
		FD_SET (camfd, &rfd);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		n = select (camfd+1, &rfd, NULL, NULL, &tv);
		if (n < 0)
		    perror ("s");
		else if (FD_ISSET (camfd, &rfd))
		    printf ("camfd is ready for reading\n");
		else
		    printf ("Nothing ready\n");
		break;

	    case 'x':	/* report settings for next exposure */
		printf ("Next settings:\n");
		printf ("     Size: %4d W x %4d H\n", cep.sw, cep.sh);
		printf ("  Binning: %4d H x %4d V\n", cep.bx, cep.by);
		printf ("Corner at: %4d   x %4d\n", cep.sx, cep.sy);
		printf (" Duration: %4d ms\n", cep.duration);
		switch (cep.shutter) {
		case CCDSO_Open: printf ("  Shutter: will be opened\n");
		    break;
		case CCDSO_Closed: printf("  Shutter: will remain closed\n");
		    break;
		case CCDSO_Dbl: printf("  Shutter: OOCO mode\n");
		    break;
		case CCDSO_Multi: printf("  Shutter: OOCOCO mode\n");
		    break;
		default: printf ("  Shutter: unknown mode\n");
		    break;
		}
		if (clipfile)
		    printf ("File clip: X=%4d Y=%4d W=%4d H=%4d\n",
			filex, filey, filew, fileh);
		break;

	    case 'z':	/* set size and location */
		clipfile = 0;
		if (sscanf (buf, "%*c %d %d %d %d",
				    &cep.sx, &cep.sy, &cep.sw, &cep.sh) != 4)
		    printf ("Bad size format\n");
		else if (setExpCCD (&cep, buf) < 0)
		    printf ("%s\n", buf);
		else
		    printf ("Ok\n");
		break;

	    default:
		help();
		break;
	    }
	}

	return (0);
}

static void
usage (char *me)
{
	fprintf (stderr, "Usage: %s [driver]\n", me);
	fprintf (stderr, "Purpose: exercise Linux CCD camera interface\n");
	fprintf (stderr, "Options:\n");
	fprintf (stderr, "  driver: path to driver.\n");
	fprintf (stderr, "          default is $TELHOME/%s\n", path_def);
	fprintf (stderr, "Type lone RETURN for command summary.\n");

	exit (1);
}

static void
help()
{
	printf ("Exposure parameters:\n");
	printf ("\tb bx by\t\tset exp binning\n");
	printf ("\td ms\t\tset exp duration\n");
	printf ("\tz sx sy sw sh\tset exp ul corner and size\n");
	printf ("\tf [fx fy fw fh]\tset full-frame, 1:1 binning,\n");
	printf ("\t\t\twith optional file ul corner and size\n");
	printf ("\th {o|c|d|m}\tshutter mode: open|closed|OOCO|OOCOCO\n");
	printf ("Direct controls:\n");
	printf ("\tC temp\t\tset cooler temp, C\n");
	printf ("\tc\t\tturn cooler off\n");
	printf ("\tm\t\tread cooler temp\n");
	printf ("\t0\t\tclose shutter now\n");
	printf ("\t1\t\topen shutter now\n");
	printf ("Info:\n");
	printf ("\tq\t\treport max image size and binning\n");
	printf ("\tx\t\treport current exp settings\n");
	printf ("Expose:\n");
	printf ("\tR\t\tdo blocking read\n");
	printf ("\tr\t\tdo non-blocking read\n");
        printf ("\tS\t\tdo a blocking select\n");
	printf ("\ts\t\tdo a non-blocking select\n");
}

/* write the file in a very simple FITS format */
static void
writeFits (CCDExpoParams *cp, char *pix)
{
	char name[64];
	int namen;
	FILE *fp;
	int fd;

	/* find a fresh name */
	for (namen = 0; ; namen++) {
	    (void) sprintf (name, "image%03d.fts", namen);
	    fp = fopen (name, "r");
	    if (!fp)
		break;
	    fclose (fp);
	}

	/* create */
	fp = fopen (name, "w");
	if (!fp) {
	    perror (name);
	    return;
	}

	/* write FITS file */
	if ((clipfile && simpleClippedFITS (fp, pix, cp) < 0) ||
						simpleFITS (fp, pix, cp) < 0)
	    fprintf (stderr, "%s: write failed\n", name);
	else
	    printf ("Wrote %s\n", name);
	fclose (fp);

	/* lame attempt to send full pathname to camera, if he's listening */
	fd = telopen ("comm/CameraFilename",
							O_WRONLY|O_NONBLOCK);
	if (fd >= 0) {
	    struct stat st;
	    if (fstat (fd, &st) == 0 && S_ISFIFO(st.st_mode)) {
		char fullpath[1024];

		getcwd (fullpath, sizeof(fullpath));
		strcat (fullpath, "/");
		strcat (fullpath, name);
		write (fd, fullpath, strlen(fullpath));
	    }
	    close (fd);
	}
}

/* create a very simple FITS file.
 * return 0 if ok, else -1
 */
static int
simpleFITS (FILE *fp, char *pix, CCDExpoParams *cp)
{
	int w = cp->sw/cp->bx;
	int h = cp->sh/cp->by;

	/* header fields */
	fitsHeader(fp, cp->sx, cp->sy, cp->sw, cp->sh, cp->bx, cp->by,
								cp->duration);

	/* pad to multiple of 2880 */
	pad_2880(fp);

	/* make pixels into big-endian 2-byte signed */
	enFITSPixels (pix, w*h);

	/* write pixels */
	fwrite (pix, w*h, sizeof(unsigned short), fp);

	/* pad to multiple of 2880 */
	pad_2880(fp);

	/* status */
	return (ferror(fp) ? -1 : 0);
}

/* create a very simple FITS file, clipped to filex/y/w/h.
 * cp defines the real pix array size.
 * N.B. we assume cp is entire chip and binning is 1:1.
 * return 0 if ok, else -1
 */
static int
simpleClippedFITS (FILE *fp, char *pix, CCDExpoParams *cp)
{
	unsigned short *cpix = (unsigned short *)pix;
	int i;

	/* check assumptions */
	if (cp->sx || cp->sy || cp->bx != 1 || cp->by != 1)
	    return (-1);

	/* header fields */
	fitsHeader(fp,filex, filey, filew, fileh, cp->bx, cp->by, cp->duration);

	/* pad to multiple of 2880 */
	pad_2880(fp);

	/* make pixels into big-endian 2-byte signed */
	enFITSPixels (pix, cp->sw*cp->sh);

	/* write pixels within filex/y/w/h */
	cpix += cp->sw*filey + filex;	/* start in ul corner */
	for (i = 0; i < fileh; i++) {
	    fwrite (cpix, filew, sizeof(unsigned short), fp);
	    cpix += cp->sw;
	}

	/* pad to multiple of 2880 */
	pad_2880(fp);

	/* status */
	return (ferror(fp) ? -1 : 0);
}

/* write header info to FITS file at fp */
static void
fitsHeader (FILE *fp, int x, int y, int w, int h, int bx, int by, int duration)
{
	CCDTempInfo tinfo;
	struct tm *tmp;
	time_t t;
	char buf[100];

	/* basic */
	sprintf(buf,"SIMPLE  =                    T / Standard FITS");
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"BITPIX  =                   16 / Bits per pixel");
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"NAXIS   =                    2 / Number of dimensions");
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"NAXIS1  =                 %4d / Number of columns", w/bx);
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"NAXIS2  =                 %4d / Number of rows", h/by);
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"BZERO   =                32768 / Real=Pixel*BSCALE+BZERO");
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"BSCALE  =                    1 / Pixel scale factor");
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"OFFSET1 =                 %4d / Image upper left x", x);
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"OFFSET2 =                 %4d / Image upper left y", y);
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"XFACTOR =                 %4d / Camera x binning factor",
									bx);
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"YFACTOR =                 %4d / Camera y binning factor",
									by);
	    fprintf (fp, "%-80s", buf);
	sprintf(buf,"EXPTIME =             %8g / Exposure time, seconds",
								duration/1000.);
	    fprintf (fp, "%-80s", buf);

	/* date and time */
	time (&t);
	tmp = gmtime (&t);
	sprintf (buf, "DATE-OBS= '%02d/%02d/%02d'           / UTC DD/MM/YY",
				    tmp->tm_mday, tmp->tm_mon+1, tmp->tm_year);
	    fprintf (fp, "%-80s", buf);
	sprintf (buf, "TIME-OBS= '%02d:%02d:%02d'           / UTC HH:MM:SS",
				    tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	    fprintf (fp, "%-80s", buf);

	/* cam temp */
	if (getTempCCD (&tinfo, buf) == 0) {
	    sprintf (buf, "CAMTEMP =                 %4d / Camera temp, C",
								    tinfo.t);
	    fprintf (fp, "%-80s", buf);
	}

	/* END */
	sprintf(buf,"END");
	    fprintf (fp, "%-80s", buf);
}

/* pad fp with blanks to the next multiple of 2880 bytes */
static void
pad_2880(FILE *fp)
{
	while (ftell(fp)%2880)
	    fprintf (fp, " ");
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: camtesthi.c,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $"};
