/* This process tries to exec telescoped.oms, else telescoped.csimc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "telenv.h"

static int pc39_open (void);

static char oms[] = "telescoped.oms";
static char csi[] = "telescoped.csi";
//static char vir[] = "telescoped.vir";

int
main (int ac, char *av[])
{
	int vfd;
	int i;
	char **ourav = malloc((ac+2)*sizeof(char*));
	
	for(i=0; i<ac; i++) {
		ourav[i] = av[i];
	}
	ourav[ac] = NULL;		
	
	/* start our log */
	telOELog (av[0]);
	
	/* If file $TELHOME/archive/config/virtual exists, we're a virtual setup */
	vfd = telopen("archive/config/virtual", O_RDONLY);
	if(vfd > 0) {
		close(vfd);
		ourav[ac] = "-v"; // virtual mode on		
		ourav[++ac] = NULL;
	}
	
	/* if pc39 opens, go with oms else csi */
	ourav[0] = pc39_open() == 0 ? oms : csi;

	daemonLog ("Starting %s%s%s%s%s%s%s\n",
				ourav[0],
				ac>1?" ":"",ac>1?ourav[1]:"",
				ac>2?" ":"",ac>2?ourav[2]:"",
				ac>3?" ":"",ac>3?ourav[3]:"",
				ac>4?" ":"",ac>4?"...":"");
		
	execvp (ourav[0], ourav);
	daemonLog ("%s%s%s%s%s%s%s: %s\n",
				ourav[0],
				ac>1?" ":"",ac>1?ourav[1]:"",
				ac>2?" ":"",ac>2?ourav[2]:"",
				ac>3?" ":"",ac>3?ourav[3]:"",
				ac>4?" ":"",ac>4?"...":"",	
				strerror(errno));
	return (1);
}

/* return 0 if pc39 driver can be opened, else -1 */
static int
pc39_open()
{
	int pc39_fd = telopen ("dev/pc39", O_RDWR);

	if (pc39_fd < 0)
	    return (-1);
	close (pc39_fd);
	return (0);
}
