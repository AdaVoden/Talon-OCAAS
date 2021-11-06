/*============================================================================
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
*   C routines for the spherical coordinate transformations used by the FITS
*   "World Coordinate System" (WCS) convention.
*
*   Summary of routines
*   -------------------
*   The spherical coordinate transformations are implemented via separate
*   functions for the transformation in each direction.
*
*   Forward transformation; sphfwd()
*   --------------------------------
*   Transform celestial coordinates to the native coordinates of a projection.
*
*   Given:
*      lng,lat  double   Celestial longitude and latitude, in degrees.
*      eul[6]   double   Euler angles for the transformation:
*                          0: Celestial longitude of the native pole, in
*                             degrees.
*                          1: Celestial colatitude of the native pole, or
*                             native colatitude of the celestial pole, in
*                             degrees.
*                          2: Native longitude of the celestial pole, in
*                             degrees.
*                          3: cos(eul[1])
*                          4: sin(eul[1])
*                          5: eul[2] - eul[0]
*
*   Returned:
*      phi,     double   Longitude and latitude in the native coordinate
*      theta             system of the projection, in degrees.
*
*   Function return value:
*               int      Error status
*                           0: Success.
*
*   Reverse transformation; sphrev()
*   --------------------------------
*   Transform native coordinates of a projection to celestial coordinates.
*
*   Given:
*      phi,     double   Longitude and latitude in the native coordinate
*      theta             system of the projection, in degrees.
*      eul[6]   double   Euler angles for the transformation:
*                          0: Celestial longitude of the native pole, in
*                             degrees.
*                          1: Celestial colatitude of the native pole, or
*                             native colatitude of the celestial pole, in
*                             degrees.
*                          2: Native longitude of the celestial pole, in
*                             degrees.
*                          3: cos(eul[1])
*                          4: sin(eul[1])
*                          5: eul[2] - eul[0]
*
*   Returned:
*      lng,lat  double   Celestial longitude and latitude, in degrees.
*
*   Function return value:
*               int      Error status
*                           0: Success.
*
*   Author: Mark Calabretta, Australia Telescope National Facility
*   $Id: sph.c,v 1.1.1.1 2003/04/15 20:46:59 lostairman Exp $
*===========================================================================*/

#include "wcstrig.h"

#ifndef __STDC__
#ifndef const
#define const
#endif
#endif


int sphfwd (lng, lat, eul, phi, theta)

const double lat, lng, eul[6];
double *phi, *theta;

{
   double coslat, coslng, dlng, sinlng, sinlat, x, y;

   coslat = cosd(lat);
   sinlat = sind(lat);

   dlng = lng - eul[0];
   coslng = cosd(dlng);
   sinlng = sind(dlng);

   /* Compute native coordinates. */
   x =  sinlat*eul[4] - coslat*eul[3]*coslng;
   y = -coslat*sinlng;
   if (x != 0.0 || y != 0.0) {
      *phi = eul[2] + atan2d(y, x);
   } else {
      /* Change of origin of longitude. */
      *phi = eul[5] + lng;
   }

   /* Normalize. */
   if (*phi > 180.0) {
      *phi -= 360.0;
   } else if (*phi < -180.0) {
      *phi += 360.0;
   }

   *theta = asind(sinlat*eul[3] + coslat*eul[4]*coslng);

   return 0;
}

/*-----------------------------------------------------------------------*/

int sphrev (phi, theta, eul, lng, lat)

const double phi, theta, eul[6];
double *lng, *lat;

{
   double cosphi, costhe, dphi, sinphi, sinthe, x, y;

   costhe = cosd(theta);
   sinthe = sind(theta);

   dphi = phi - eul[2];
   cosphi = cosd(dphi);
   sinphi = sind(dphi);

   /* Compute celestial coordinates. */
   x =  sinthe*eul[4] - costhe*eul[3]*cosphi;
   y = -costhe*sinphi;
   if (x != 0.0 || y != 0.0) {
      *lng = eul[0] + atan2d(y, x);
   } else {
      /* Change of origin of longitude. */
      *lng = phi - eul[5];
   }

   /* Normalize. */
   if (*lng > 360.0) {
      *lng -= 360.0;
   } else if (*lng < -360.0) {
      *lng += 360.0;
   }

   *lat = asind(sinthe*eul[3] + costhe*eul[4]*cosphi);

   return 0;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: sph.c,v $ $Date: 2003/04/15 20:46:59 $ $Revision: 1.1.1.1 $ $Name:  $"};
