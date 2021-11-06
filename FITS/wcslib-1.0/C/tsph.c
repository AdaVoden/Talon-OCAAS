/*============================================================================
*
*   WCSLIB - an implementation of the FITS WCS proposal.
*   Copyright (C) 1995, Mark Calabretta
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by the
*   Free Software Foundation; either version 2 of the License, or (at your
*   option) any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   You should have received a copy of the GNU General Public License along
*   with this library; if not, write to the Free Software Foundation, Inc.,
*   675 Mass Ave, Cambridge, MA 02139, USA.
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
*   tsph tests the forward and reverse spherical coordinate transformation
*   routines for closure.
*
*   Poor-closure is sometimes found for positions near the native pole and
*   should not be considered a problem.
*
*   $Id: tsph.c,v 1.1.1.1 2003/04/15 20:47:00 lostairman Exp $
*---------------------------------------------------------------------------*/

#include "wcstrig.h"

#ifndef __STDC__
#ifndef const
#define const
#endif
#endif


main()

{
   int   ierr, lat, lng;
   double coslat, lng1, lng2, eul[6], lat1, lat2, phi, theta;
   const double tol = 1.0e-12;

   /* Set reference angles. */
   eul[0] =  90.0;
   eul[1] =  30.0;
   eul[2] = -90.0;
   printf("%s\n%s\n%s%10.4lf%10.4lf%10.4lf\n",
      "Testing closure of the spherical coordinate transformation routines.",
      "Celestial longitude and latitude of the native pole, and native",
      "longitude of the celestial pole (degrees):", eul[0], eul[1], eul[2]);

   eul[3] = cosd(eul[1]);
   eul[4] = sind(eul[1]);
   eul[5] = eul[2] - eul[0];

   printf ("Closure tolerance:%8.1lg degrees of arc.\n", tol);

   for (lat = 90; lat >= -90; lat--) {
      lat1 = (double)lat;
      coslat = cosd(lat1);

      for (lng = -180; lng <= 180; lng++) {
         lng1 = (double)lng;

         if (ierr = sphfwd (lng1, lat1, eul, &phi, &theta)) {
            printf ("lng1 =%20.15lf  lat1 =%20.15lf  Error%3d\n",
               lng1, lat1, ierr);
            continue;
         }

         if (ierr = sphrev (phi, theta, eul, &lng2, &lat2)) {
            printf("lng1 =%20.15lf  lat1 =%20.15lf\n", lng1, lat1);
            printf(" phi =%20.15lf theta =%20.15lf  Error%3d\n",
                   phi, theta, ierr);
            continue;
         }

         if (fabs(lat2-lat1) > tol || (fabs(lng2-lng1)-360.0)*coslat > tol) {
            printf("Unclosed: lng1 =%20.15lf  lat1 =%20.15lf\n", lng1, lat1);
            printf("           phi =%20.15lf theta =%20.15lf\n", phi, theta);
            printf("          lng2 =%20.15lf  lat2 =%20.15lf\n", lng2, lat2);
         }
      }
   }

   return 0;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: tsph.c,v $ $Date: 2003/04/15 20:47:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
