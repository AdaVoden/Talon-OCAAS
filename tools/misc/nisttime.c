/*
 * Get time information from a NIST server
 * and report in a variety of formats
 *
 * Originally from udp_time_client
 *
 */

/*
	client process to connect to  the time service via port 37 using
	udp.  The client process uses the time message received to check 
	(and optionally to set) the time of the local clock.  the comparison 
	assumes that the local clock keeps time in seconds from 1/1/70 
	which is the UNIX standard, and it assumes that the received time
	is in seconds since 1900.0 to conform to rfc868.  It computes the
	time difference by subtracting 2208988800 from the received time
	to convert it to seconds since 1970 and then makes the comparison
	directly. If the local machine keeps time in some other way, then 
	the comparison method will have to change, but the rest should be 
	okay.

	This software was developed with US Government support
	and it may not be sold, restricted or licensed.  You 
	may duplicate this program provided that this notice
	remains in all of the copies, and you may give it to
	others provided they understand and agree to this
	condition.

	This program and the time protocol it uses are under 
	development and the implementation may change without 
	notice.

	For questions or additional information, contact:

	Judah Levine
	Time and Frequency Division
	NIST/847
	325 Broadway
	Boulder, Colorado 80303
	(303) 492 7785
	jlevine@time_a.timefreq.bldrdoc.gov
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "strops.h"
#include "telenv.h"
#include "misc.h"

long int address;		/* holds ip address */
int pserv = 37;			/* daytime port number on server */
char *cp;			/* server address in . notation */
struct sockaddr_in s_in;	/* socket address structure */
int s;				/* socket number */
int length;			/* size of message */
char buf[10];			/* holds message */
long recdtime;		/* received time in local byte order */
long dorg = 2208988800U;	/*seconds from 1900.0 -> 1970.0*/
long netcons;		/*received time in network order */
struct timeval tvv,tgg;		/* holds local time */
long int diff;			/* time difference, local - NIST */
char cc;
int stat;
int weareroot;

int tflag,lflag,vflag,dflag,sflag;
char *me;

static void usage (void);

/*------------------*/

int main (int ac, char *av[])
{
/*
	Defaults
*/
	cp="131.107.1.10"; // time-nw.nist.gov
	tflag=0;
	lflag=0;
	vflag=0;
	dflag=0;

	me = av[0]; //basenm(av[0]);
	
	if(ac < 2)	usage();
		
	while ((--ac > 0) && ((*++av)[0] == '-')) {
	    char *sw;
	    for (sw = av[0]+1; *sw != '\0'; sw++) {
			switch (*sw) {
				case 't':
				    tflag++;
				    break;
				case 'l':
					lflag++;
					break;
				case 'v':
					vflag++;
					break;
				case 'd':
					dflag++;
					break;
				case 's':
					sflag++;
					break;
				case 'a':
					cp=*++av;
					ac--;
					break;
				default:
				    usage();
			}			
		}
	}
			
/*
	convert address to internal format
*/
	if( (address=inet_addr(cp) ) == -1)
	   {
	   fprintf(stderr,"Internet address error.\n");
	   exit(1);
	}
	memset((void *)&s_in, 0, sizeof(s_in));
	s_in.sin_addr.s_addr=address;
	s_in.sin_family=AF_INET;
	s_in.sin_port=htons(pserv);
/*
	create the socket and then connect to the server
	note that this is a UDP datagram socket 
*/
	if( (s=socket(AF_INET,SOCK_DGRAM,0) ) < 0)
	   {
	   fprintf(stderr,"Socket creation error.\n");
	   exit(1);
	}
	if(connect(s, (struct sockaddr *) &s_in, sizeof(s_in) ) < 0)
	   {
	   perror(" time server Connect failed --");
	   exit(1);
	}
/*
	send a UDP datagram to start the server 
	going.
*/
	write(s,buf,10);
	length=read(s,&netcons,sizeof(netcons));
	gettimeofday(&tvv,0);	/*get time as soon as read completes*/
	if(length <= 0) 
	   {
	   fprintf(stderr,"No response from server.\n");
	   close(s);
	   exit(1);
	}
	recdtime=ntohl(netcons);	/*convert to local byte order */
	recdtime -= dorg;	/*convert to seconds since 1970*/
	close(s);
	
	if(sflag) {
		if(vflag) printf("Server inet addr: ");
		printf("%s\n",cp);
	}
	
	if(tflag) {
		if(vflag) printf("Server Date/Time: ");
		printf ("%s", ctime (&recdtime));	/* includes \n */
	}
	
	if(lflag) {
		if(vflag) printf("System Date/Time: ");
		printf("%s", ctime (&tvv.tv_sec));	/* includes \n */
	}			
/*
	now compute difference between local time and
	received time.
*/
	diff = tvv.tv_sec - recdtime;
	if(tvv.tv_usec >= 500000l) diff++;	/*round microseconds */
	
	if(dflag) {
		if(vflag) printf("Local Clock - NIST = ");
		printf("%ld",diff);
		if(vflag) printf(" second(s).");
		printf("\n");
	}
	
	return (0);
}

static void usage (void)
{
    FILE *fp = stderr;
    char *p = me;

    fprintf(fp,"Usage: %s: -[tldv][a <ip>]\n", p);
    fprintf(fp,"Purpose: Show time as reported by NIST server\n");
    fprintf(fp,"-t          : show the NIST date/time\n");
    fprintf(fp,"-l          : show the local system date/time\n");
    fprintf(fp,"-d          : show difference in seconds\n");
    fprintf(fp,"-a x.x.x.x  : ip of server (dotted quad format only)\n");
    fprintf(fp,"-v          : verbose (show labels)\n");

    exit(1);
}
