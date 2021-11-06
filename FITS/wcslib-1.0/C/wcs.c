/*=============================================================================
*
*   WCSLIB - an implementation of the FITS WCS proposal.
*   Copyright (C) 1995, Mark Calabretta
*
*   This library is free software; you can redistribute it and/or modify it
*   under the terms of the GNU Library General Public License as published
*   by the Free Software Foundation; either version 2 of the License, or (at
*   your option) any later version.
*
*   This library is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
*   General Public License for more details.
*
*   You should have received a copy of the GNU Library General Public License
*   along with this library; if not, write to the Free Software Foundation,
*   Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Correspondence concerning WCSLIB may be directed to:
*      Internet email: mcalabre@atnf.csiro.au
*      Postal address: Dr. Mark Calabretta,
*                      Australia Telescope National Facility,
*                      P.O. Box 76,
*                      Epping, NSW, 2121,
*                      AUSTRALIA
*
*=============================================================================
*
*   C routines which implement the FITS World Coordinate System (WCS)
*   convention.
*
*   Summary of routines
*   -------------------
*   These routines are provided as drivers for the lower level spherical
*   coordinate transformation and projection routines.  There are separate
*   driver routines for the forward (wcsfwd) and reverse (wcsrev)
*   transformations.  A third routine (wcsset) initializes transformation
*   parameters but need not be called explicitly - see the explanation of
*   wcs.flag below.
*
*   Forward transformation; wcsfwd
*   ------------------------------
*   Compute (x,y) coordinates in the plane of projection from celestial
*   coordinates (lng,lat).
*
*   Given:
*      pcode[4] char     WCS projection code (see below)
*      lng,lat  double   Celestial longitude and latitude of the projected
*                        point, in degrees.
*
*   Given and returned:
*      wcs      wcsprm*  Spherical coordinate transformation parameters
*                        (see below)
*      prj      prjprm*  Projection parameters (usage is described in the
*                        prologue to "proj.c").
*
*   Returned:
*      x,y      double*  Projected coordinates, "degrees".
*
*   Function return value:
*               int      Error status
*                           0: Success.
*                           1: Invalid coordinate transformation parameters.
*                           2: Invalid projection parameters.
*                           3: Invalid value of (lng,lat).
*
*   Reverse transformation; wcsrev
*   ------------------------------
*   Compute the celestial coordinates (lng,lat) of the point with projected
*   coordinates (x,y).
*
*   Given:
*      pcode[4] char     WCS projection code.
*      x,y      double   Projected coordinates, "degrees".
*
*   Given and returned:
*      wcs      wcsprm*  Coordinate transformation parameters (see below)
*      prj      prjprm*  Projection parameters (usage is described in the
*                        prologue to "proj.c").
*
*   Returned:
*      lng,lat  double*  Celestial longitude and latitude of the projected
*                        point, in degrees.
*
*   Function return value:
*               int      Error status
*                           0: Success.
*                           1: Invalid coordinate transformation parameters.
*                           2: Invalid projection parameters.
*                           3: Invalid value of (x,y).
*
*   WCS projection codes
*   --------------------
*   Zenithals/azimuthals:
*      AZP: zenithal/azimuthal perspective
*      TAN: gnomonic
*      SIN: synthesis (generalized orthographic)
*      STG: stereographic
*      ARC: zenithal/azimuthal equidistant
*      ZPN: zenithal/azimuthal polynomial
*      ZEA: zenithal/azimuthal equal area
*      AIR: Airy
*
*   Cylindricals:
*      CYP: cylindrical perspective
*      CAR: Cartesian
*      MER: Mercator
*      CEA: cylindrical equal area
*
*   Conics:
*      COP: conic perspective
*      COD: conic equidistant
*      COE: conic equal area
*      COO: conic orthomorphic
*
*   Polyconics:
*      BON: Bonne
*      PCO: polyconic
*
*   Pseudo-cylindricals:
*      GLS: Sanson-Flamsteed (global sinusoidal)
*      PAR: parabolic
*      MOL: Mollweide
*
*   Conventional:
*      AIT: Hammer-Aitoff
*
*   Quad-cubes:
*      CSC: COBE quadrilateralized spherical cube
*      QSC: quadrilateralized spherical cube
*      TSC: tangential spherical cube
*
*   Coordinate transformation parameters; wcs
*   -----------------------------------------
*   The wcsprm struct consists of the following:
*
*      wcs.ref[3]
*         The first pair of values should be set to the celestial longitude
*         and latitude (usually right ascension and declination) of the
*         reference point of the projection.  The third value is the native
*         longitude of the pole of the celestial coordinate system which
*         corresponds to the FITS keyword LONGPOLE.  Note that this has no
*         default value and should normally be set to 180 degrees.
*      wcs.flag
*         This flag must be set to zero whenever any of wcs.ref[3] are set or
*         changed.  This signals the initialization routine (wcsset) to
*         recompute intermediaries.
*
*   The remaining members of the wcsprm struct are maintained by the
*   initialization routines and should not be modified.  This is done for the
*   sake of efficiency and to allow an arbitrary number of contexts to be
*   maintained simultaneously.
*
*      wcs.euler[6]
*         Euler angles and associated intermediaries derived from the
*         coordinate reference values.
*      wcs.prjfwd
*      wcs.prjrev
*         Pointers to the forward and reverse projection routines.
*
*   Author: Mark Calabretta, Australia Telescope National Facility
*   $Id: wcs.c,v 1.1.1.1 2003/04/15 20:47:00 lostairman Exp $
*===========================================================================*/

#include "wcs.h"

int wcsset(pcode, wcs)

char pcode[4];
struct wcsprm *wcs;

{
   int    iref;
   double cphip, slat0, x, y;

   /* Set pointers to the forward and reverse projection routines. */
   if (strcmp(pcode, "AZP") == 0) {
      wcs->prjfwd = azpfwd;
      wcs->prjrev = azprev;
      iref = 0;
   } else if (strcmp(pcode, "TAN") == 0) {
      wcs->prjfwd = tanfwd;
      wcs->prjrev = tanrev;
      iref = 0;
   } else if (strcmp(pcode, "SIN") == 0) {
      wcs->prjfwd = sinfwd;
      wcs->prjrev = sinrev;
      iref = 0;
   } else if (strcmp(pcode, "STG") == 0) {
      wcs->prjfwd = stgfwd;
      wcs->prjrev = stgrev;
      iref = 0;
   } else if (strcmp(pcode, "ARC") == 0) {
      wcs->prjfwd = arcfwd;
      wcs->prjrev = arcrev;
      iref = 0;
   } else if (strcmp(pcode, "ZPN") == 0) {
      wcs->prjfwd = zpnfwd;
      wcs->prjrev = zpnrev;
      iref = 0;
   } else if (strcmp(pcode, "ZEA") == 0) {
      wcs->prjfwd = zeafwd;
      wcs->prjrev = zearev;
      iref = 0;
   } else if (strcmp(pcode, "AIR") == 0) {
      wcs->prjfwd = airfwd;
      wcs->prjrev = airrev;
      iref = 0;
   } else if (strcmp(pcode, "CYP") == 0) {
      wcs->prjfwd = cypfwd;
      wcs->prjrev = cyprev;
      iref = 1;
   } else if (strcmp(pcode, "CAR") == 0) {
      wcs->prjfwd = carfwd;
      wcs->prjrev = carrev;
      iref = 1;
   } else if (strcmp(pcode, "MER") == 0) {
      wcs->prjfwd = merfwd;
      wcs->prjrev = merrev;
      iref = 1;
   } else if (strcmp(pcode, "CEA") == 0) {
      wcs->prjfwd = ceafwd;
      wcs->prjrev = cearev;
      iref = 1;
   } else if (strcmp(pcode, "COP") == 0) {
      wcs->prjfwd = copfwd;
      wcs->prjrev = coprev;
      iref = 0;
   } else if (strcmp(pcode, "COD") == 0) {
      wcs->prjfwd = codfwd;
      wcs->prjrev = codrev;
      iref = 0;
   } else if (strcmp(pcode, "COE") == 0) {
      wcs->prjfwd = coefwd;
      wcs->prjrev = coerev;
      iref = 0;
   } else if (strcmp(pcode, "COO") == 0) {
      wcs->prjfwd = coofwd;
      wcs->prjrev = coorev;
      iref = 0;
   } else if (strcmp(pcode, "BON") == 0) {
      wcs->prjfwd = bonfwd;
      wcs->prjrev = bonrev;
      iref = 1;
   } else if (strcmp(pcode, "PCO") == 0) {
      wcs->prjfwd = pcofwd;
      wcs->prjrev = pcorev;
      iref = 1;
   } else if (strcmp(pcode, "GLS") == 0) {
      wcs->prjfwd = glsfwd;
      wcs->prjrev = glsrev;
      iref = 1;
   } else if (strcmp(pcode, "PAR") == 0) {
      wcs->prjfwd = parfwd;
      wcs->prjrev = parrev;
      iref = 1;
   } else if (strcmp(pcode, "AIT") == 0) {
      wcs->prjfwd = aitfwd;
      wcs->prjrev = aitrev;
      iref = 1;
   } else if (strcmp(pcode, "MOL") == 0) {
      wcs->prjfwd = molfwd;
      wcs->prjrev = molrev;
      iref = 1;
   } else if (strcmp(pcode, "CSC") == 0) {
      wcs->prjfwd = cscfwd;
      wcs->prjrev = cscrev;
      iref = 1;
   } else if (strcmp(pcode, "QSC") == 0) {
      wcs->prjfwd = qscfwd;
      wcs->prjrev = qscrev;
      iref = 1;
   } else if (strcmp(pcode, "TSC") == 0) {
      wcs->prjfwd = tscfwd;
      wcs->prjrev = tscrev;
      iref = 1;
   } else {
      /* Unrecognized projection code. */
      return 1;
   }

   /* Compute celestial coordinates of the native pole. */
   if (iref == 0) {
      /* Reference point is at the native pole. */
      wcs->euler[0] = wcs->ref[0];
      wcs->euler[1] = 90.0 - wcs->ref[1];
   } else {
      /* Reference point is at the native origin. */
      slat0 = sind(wcs->ref[1]);
      cphip = cosd(wcs->ref[2]);
      if (cphip == 0.0) {
         if (slat0 != 0.0) {
            return 1;
         }
         wcs->euler[1] = 90.0;
      } else {
         if (fabs(slat0/cphip) > 1.0) {
            return 1;
         }
         wcs->euler[1] = asind(slat0/cphip);
         if (wcs->euler[1] < 0.0) wcs->euler[1] += 180.0;
      }

      x = -cphip*cosd(wcs->euler[1]);
      y =  sind(wcs->ref[2]);
      if (x == 0.0 && y == 0.0) {
         if (fmod(wcs->ref[2],360.0) == 0.0) {
            wcs->euler[0] = wcs->ref[0] - 90.0;
         } else {
            wcs->euler[0] = wcs->ref[0] + 90.0;
         }
      } else {
         wcs->euler[0] = wcs->ref[0] - atan2d(y,x);
      }
   }
   wcs->euler[2] = wcs->ref[2];
   wcs->euler[3] = cosd(wcs->euler[1]);
   wcs->euler[4] = sind(wcs->euler[1]);
   wcs->euler[5] = wcs->euler[2] - wcs->euler[0];
   wcs->flag = WCSSET;

   return 0;
}

/*--------------------------------------------------------------------------*/

int wcsfwd(pcode, lng, lat, wcs, prj, x, y)

char   pcode[4];
double lng, lat;
struct wcsprm *wcs;
struct prjprm *prj;
double *x, *y;

{
   int    err;
   double phi, theta;

   if (wcs->flag == 0) {
      if (wcsset(pcode, wcs)) return 1;
   }

   /* Compute native coordinates. */
   sphfwd(lng, lat, wcs->euler, &phi, &theta);

   /* Apply forward projection. */
   if (err = wcs->prjfwd(phi, theta, prj, x, y)) {
      return err == 1 ? 2 : 3;
   }

   return 0;
}

/*--------------------------------------------------------------------------*/

int wcsrev(pcode, x, y, prj, wcs, lng, lat)

char   pcode[4];
double x, y;
struct prjprm *prj;
struct wcsprm *wcs;
double *lng, *lat;

{
   int    err;
   double phi, theta;

   if (wcs->flag == 0) {
      if(wcsset(pcode, wcs)) return 1;
   }

   /* Apply reverse projection. */
   if (err = wcs->prjrev(x, y, prj, &phi, &theta)) {
      return err == 1 ? 2 : 3;
   }

   /* Compute native coordinates. */
   sphrev(phi, theta, wcs->euler, lng, lat);

   return 0;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: wcs.c,v $ $Date: 2003/04/15 20:47:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
