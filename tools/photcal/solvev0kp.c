#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "photstd.h"

#include "photcal.h"

/* solve for the best V and kp which match the star data in sp and the
 * fixed values of [BVRI]kpp. Indeces into V[] and kp[] are in order of BVRI.
 * since there are just two unknowns (for each color), it boils down to solving
 *   for the best k' and V0 in:
 *      V - k''(B-V) - Vobs = k'Z + V0
 *   which is the form of a best-fit line: y = mx + b where m==kp and b==V0.
 *   (when line is written as y = a + Bx then a==V0 and b==kp)
 * return 0 if all ok, else -1.
 */
int
solve_V0_kp (PStdStar *sp, int nsp, double Bkpp, double Vkpp, double Rkpp,
    double Ikpp, double V0[], double V0err[], double kp[], double kperr[])
{
	double B, Bx, Bxx, Bxy, By;	/* B sums */
	double V, Vx, Vxx, Vxy, Vy;	/* V sums */
	double R, Rx, Rxx, Rxy, Ry;	/* R sums */
	double I, Ix, Ixx, Ixy, Iy;	/* I sums */
	int nb, nv, nr, ni;		/* counts */
	double d, m, b;
	PStdStar *stp;
	int ok;

	/* be optimistic */
	ok = 1;

	/* scan all the stars and compute the various sums for least sqrs */
	nb = nv = nr = ni = 0;
	B = Bx = Bxx = Bxy = By = 0.0;
	V = Vx = Vxx = Vxy = Vy = 0.0;
	R = Rx = Rxx = Rxy = Ry = 0.0;
	I = Ix = Ixx = Ixy = Iy = 0.0;
	for (stp = sp; --nsp >= 0; stp++) {
	    double x, y, e;
	    int i;

	    for (i = 0; i < stp->nB; i++) {
		OneStar *op = &stp->Bp[i];

		e = op->Verr * op->Verr;
		x = op->Z;
		y = stp->Bm - Bkpp*(stp->Bm - stp->Vm) - op->Vobs;
		B += 1/e; Bx += x/e; Bxx += x*x/e; Bxy += x*y/e; By += y/e;
		nb++;
	    }

	    for (i = 0; i < stp->nV; i++) {
		OneStar *op = &stp->Vp[i];

		e = op->Verr * op->Verr;
		x = op->Z;
		y = stp->Vm - Vkpp*(stp->Bm - stp->Vm) - op->Vobs;
		V += 1/e; Vx += x/e; Vxx += x*x/e; Vxy += x*y/e; Vy += y/e;
		nv++;
	    }

	    for (i = 0; i < stp->nR; i++) {
		OneStar *op = &stp->Rp[i];

		e = op->Verr * op->Verr;
		x = op->Z;
		y = stp->Rm - Rkpp*(stp->Bm - stp->Vm) - op->Vobs;
		R += 1/e; Rx += x/e; Rxx += x*x/e; Rxy += x*y/e; Ry += y/e;
		nr++;
	    }

	    for (i = 0; i < stp->nI; i++) {
		OneStar *op = &stp->Ip[i];

		e = op->Verr * op->Verr;
		x = op->Z;
		y = stp->Im - Ikpp*(stp->Bm - stp->Vm) - op->Vobs;
		I += 1/e; Ix += x/e; Ixx += x*x/e; Ixy += x*y/e; Iy += y/e;
		ni++;
	    }
	}

	/* now we can solve for the V and kp and their errors */
	if (nb > 0) {
	    d = B*Bxx - Bx*Bx;
	    m = (B*Bxy - Bx*By)/d;
	    b = (Bxx*By - Bx*Bxy)/d;
	    kp[0] = m;
	    kperr[0] = sqrt(B/d);
	    V0[0] = b;
	    V0err[0] = sqrt(Bxx/d);
	    if (verbose > 1)
		printf ("nb=%d d=%g B=%g Bx=%g By=%g Bxx=%g Bxy=%g\n",
							nb,d,B,Bx,By,Bxx,Bxy);
	} else {
	    fprintf (stdout, "No B data found.\n");
	    ok = 0;
	}

	if (nv > 0) {
	    d = V*Vxx - Vx*Vx;
	    m = (V*Vxy - Vx*Vy)/d;
	    b = (Vxx*Vy - Vx*Vxy)/d;
	    kp[1] = m;
	    kperr[1] = sqrt(V/d);
	    V0[1] = b;
	    V0err[1] = sqrt(Vxx/d);
	    if (verbose > 1)
		printf ("nv=%d d=%g V=%g Vx=%g Vy=%g Vxx=%g Vxy=%g\n",
							nv,d,V,Vx,Vy,Vxx,Vxy);
	} else {
	    fprintf (stdout, "No V data found.\n");
	    ok = 0;
	}

	if (nr > 0) {
	    d = R*Rxx - Rx*Rx;
	    m = (R*Rxy - Rx*Ry)/d;
	    b = (Rxx*Ry - Rx*Rxy)/d;
	    kp[2] = m;
	    kperr[2] = sqrt(R/d);
	    V0[2] = b;
	    V0err[2] = sqrt(Rxx/d);
	    if (verbose > 1)
		printf ("nr=%d d=%g R=%g Rx=%g Ry=%g Rxx=%g Rxy=%g\n",
							nr,d,R,Rx,Ry,Rxx,Rxy);
	} else {
	    fprintf (stdout, "No R data found.\n");
	    ok = 0;
	}

	if (ni > 0) {
	    d = I*Ixx - Ix*Ix;
	    m = (I*Ixy - Ix*Iy)/d;
	    b = (Ixx*Iy - Ix*Ixy)/d;
	    kp[3] = m;
	    kperr[3] = sqrt(I/d);
	    V0[3] = b;
	    V0err[3] = sqrt(Ixx/d);
	    if (verbose > 1)
		printf ("ni=%d d=%g I=%g Ix=%g Iy=%g Ixx=%g Ixy=%g\n",
							ni,d,I,Ix,Iy,Ixx,Ixy);
	} else {
	    fprintf (stdout, "No I data found.\n");
	    ok = 0;
	}

	return (ok ? 0 : -1);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: solvev0kp.c,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $"};
