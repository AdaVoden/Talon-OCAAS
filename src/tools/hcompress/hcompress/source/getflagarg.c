/* EXTRACT BINARY FLAG PARAMETERS FROM COMMAND LINE ARGUMENTS */

#include <string.h>

/* returns 1 if flag is present, else 0 */

extern int
getflagarg(argc,argv,flag)
int argc; 
char *argv[];
char flag[];								/* flag (e.g. c for "-c")		*/
{
int arg;
char *p;

	for (arg = 1; arg<argc; arg++) {
		/*
		 * look for -flag
		 */
		p = argv[arg];
		if (*p++ == '-' && (strcmp(p,flag) == 0)) return(1);
	}
	return(0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: getflagarg.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
