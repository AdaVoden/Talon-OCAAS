
/* Utility to extract values from a FITS file header

	S. Ohmert 4/25/02
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "fits.h"

static void usage(char *pname);

int main (int argc, char **argv)
{
	char *progname;
	char *filename;
	char *keyname;
	char xstring[80];
	double xdouble;
	int	xint;
	char errmsg[512];
	int   ndx;
	FImage fimage, *fip = &fimage;
	int fd;
		
	progname = argv[0];
	
	if(argc < 2 || argv[1][0] == '-') {
		usage(progname);
	}
	
	
	filename = argv[1];	
	fd = open(filename,O_RDONLY);	
	if(fd < 0) {
		fprintf(stderr,"Unable to open %s\n",filename);
		exit(1);
	}
	
	initFImage (fip);

	/* read header, or whole thing if from stdin which we truely copy */
	if (fd == 0) {
	    if (readFITS (fd, fip, errmsg) < 0) {
			fprintf (stderr, "%s: %s\n", filename, errmsg);
			close(fd);
			return(1);
	    }
	} else {
	    if (readFITSHeader (fd, fip, errmsg) < 0) {
			fprintf (stderr, "%s: %s\n", filename, errmsg);
			close(fd);
			return(1);
	    }
	}
	
	close(fd);
			
	ndx = 1;	
	while(++ndx < argc) {
		keyname = argv[ndx];
		if(getStringFITS(fip,keyname,xstring) >=0 ) {
			fprintf(stdout, "%s\n",xstring);
		}
		else if(getRealFITS(fip, keyname, &xdouble) >=0) {
			if(getIntFITS(fip, keyname, &xint) >= 0) {
				if((double) xint == xdouble) {
					fprintf(stdout,"%d\n",xint);
					continue;
				}
			}
			fprintf(stdout,"%.12f\n",xdouble);
		}							
		else fprintf(stdout,"\n");
	}	

	return(0);
}

static void usage(char *pname)
{
	fprintf(stderr,"Usage: %s fitsfile [keywords]\n",pname);
	fprintf(stderr,"  This will return a value on a separate line for each field given.\n");
	fprintf(stderr,"  If the field is not found, a blank line is returned.\n");
}
