/* this is a public domain version of getopt */

#include <stdio.h>
#include <string.h>

#define ERR(ms,cc) if(opterr) {(void) fprintf(stderr,"%s%s%c\n",argv[0],ms,cc);}

int  opterr = 1;
int  optind = 1;
int  optopt;
char *optarg;

extern int
getopt(argc, argv, opts)
int  argc;
char *const *argv;
const char *opts;
{
static int sp = 1;
int c;
char *cp;

	if(sp == 1) {
		if(optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == NULL) {
		ERR(": unknown option, -", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": argument missing for -", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: getopt.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
