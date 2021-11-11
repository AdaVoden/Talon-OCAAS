///////////////////////////////////////
//
// Focustemp -- a utility to manage
// focus position to temperature interpolation data
//
//////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "misc.h"
#include "running.h"
#include "strops.h"
#include "telenv.h"
#include "scan.h"
#include "telstatshm.h"
#include "configfile.h"
#include "cliserv.h"

#include "focustemp.h"

static TelStatShm *telstatshmp;

static void usage(void);
void listFocPosData(char *filter);
void addFocPosData(char *filter, double tmp, double position);

char curFilter[2] = {'?','\0'};
double curTmp;
char *progname;

int main (int ac, char *av[])
{
	double tmp,position;
	char *filter;
	
	progname = basenm(av[0]);
	
	// note: telstatshmp will be NULL if this fails
	if(open_telshm(&telstatshmp) >= 0) {
		curTmp = telstatshmp->now.n_temp;
		curFilter[0] = telstatshmp->filter;
	}		
		
	/* look for command. If none recognized, default is "get" */
	if(ac > 1 && *av[1] == '-') {
		if(!strcasecmp(av[1],"--help") || !strcasecmp(av[1],"-?") || !strcasecmp(av[1],"-h")) {
			usage();
		}
		else if(!strcasecmp(av[1],"--add") || !strcasecmp(av[1],"-a")) {
    		if(ac == 5) {
		    	char *chkp;
				filter = av[2];				
				tmp = strtod(av[3],&chkp);
				if(tmp == 0 && chkp == av[3]) {
					fprintf(stderr,"Bad value for temperature: %s\n",av[3]);
					usage();
				}
				position = strtod(av[4],&chkp);
				if(tmp == 0 && chkp == av[4]) {
					fprintf(stderr,"Bad value for position: %s\n",av[4]);
					usage();
				}
				addFocPosData(filter,tmp,position);
			
			} else {
				fprintf(stderr,"Wrong number of arguments for %s option\n",av[1]);
				usage();			
			}				
		}			
		else if(!strcasecmp(av[1],"--list") || !strcasecmp(av[1],"-l")) {
			if(ac == 3) {
				filter = av[2];
			} else {
				filter = NULL;
			}
			listFocPosData(filter);
		}
	}
	else if(ac > 1 && *av[1] == '-') {
		fprintf(stderr,"Unrecognized option: %s\n",av[1]);
		usage();
	}
	else {
		// optional filter and temp parameters for "get" (no command)
		filter = (ac >= 2) ? av[1] : curFilter;
		if(ac >= 3) {
			char *chkp;
			tmp = strtod(av[2],&chkp);
			if(tmp == 0 && chkp == av[2]) {
				fprintf(stderr,"Bad value for temperature: %s\n",av[2]);
				usage();
			}
		} else {
			tmp = curTmp;
		}
		if(focusPositionReadData() < 0) {
			exit(1);
		}
		if(focusPositionFind(*filter,tmp,&position) != 0) {
			fprintf(stderr,"No data exists for filter %c\n",*filter);
			exit(1);
		}
		else {
			fprintf(stdout,"%.2lf\n",position);
		}
	}
	
	return 0;
}		

static void usage(void)
{
	FILE *fp = stderr;

	fprintf(fp,"\n%s usage\n", progname);
	fprintf(fp," To get the position for current filter at current temperature:\n");
	fprintf(fp,"    %s\n", progname);
	fprintf(fp," To get the position for given filter at current temperature:\n");
	fprintf(fp,"    %s <filter>\n",progname);
	fprintf(fp," To get the position for given filter at given temperature:\n");
	fprintf(fp,"    %s <filter> <temp>\n",progname);
	fprintf(fp," To add a new data entry:\n");
	fprintf(fp,"    %s --add <filter> <temp> <position>\n",progname);
	fprintf(fp," To list all entries for the current filter:\n");
	fprintf(fp,"    %s --list\n",progname);
	fprintf(fp," To list all entries for a given filter:\n");
	fprintf(fp,"    %s --list <filter>\n",progname);
	fprintf(fp,"\n");
	exit (1);
}

void listFocPosData(char *filter)
{
//	if(!filter) 	TRACE "Listing all data!\n");
//	else 			TRACE "Listing data for %c\n",*filter);
	
	if(focusPositionReadData() >= 0) {
		FocPosEntry *pt = getFocusPositionTable();
		int entries = getFocusPositionEntries();
//		TRACE "There are %d entries\n",entries);
		while(entries--) {
			if(!filter || *filter == pt->filter) {
				fprintf(stdout,"%c %.2lf %.2lf\n",pt->filter,pt->tmp,pt->position);
			}
			pt++;
		}
	}
}
void addFocPosData(char *filter, double tmp, double position)
{
//	TRACE "Adding filter data:\n%c %.2lf %.2lf\n",*filter,tmp,position);
	if(focusPositionReadData() < 0) {
		focusPositionClearTable();
	}
	focusPositionAdd(*filter, tmp, position);
	focusPositionWriteData();
}
