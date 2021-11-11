/* undigitize.c		undigitize H-transform
 *
 * Programmer: R. White		Date: 9 May 1991
 */
#include <stdio.h>

extern void
undigitize(a,nx,ny,scale)
int a[];
int nx,ny;
int scale;
{
int *p;

	/*
	 * multiply by scale
	 */
	if (scale <= 1) return;
	for (p=a; p <= &a[nx*ny-1]; p++) *p = (*p)*scale;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: undigitize.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
