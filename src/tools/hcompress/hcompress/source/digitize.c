/* digitize.c	digitize H-transform
 *
 * Programmer: R. White		Date: 11 March 1991
 */
#include <stdio.h>

extern void
digitize(a,nx,ny,scale)
int a[];
int nx,ny;
int scale;
{
int d, *p;

	/*
	 * round to multiple of scale
	 */
	if (scale <= 1) return;
	d=(scale+1)/2-1;
	for (p=a; p <= &a[nx*ny-1]; p++) *p = ((*p>0) ? (*p+d) : (*p-d))/scale;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: digitize.c,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $"};
