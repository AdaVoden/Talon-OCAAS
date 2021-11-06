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
*   twcs tests WCSLIB by drawing native and celestial coordinate grids for
*   Bonne's projection.
*
*   $Id: twcs.c,v 1.1.1.1 2003/04/15 20:47:00 lostairman Exp $
*---------------------------------------------------------------------------*/

#include "wcs.h"

main()

{
   char   pcode[4], text[80];
   int    ci, ierr, ilat, ilng, j;
   float  xr[512], yr[512];
   double lat, lat0, lng, lng0, phi0, x, y;
   struct wcsprm native, celestial;
   struct prjprm prj;

   /* Set up Bonne's projection with reference latitude at +30. */
   strcpy(pcode, "BON");

   /* Initialize projection parameters. */
   prj.flag = 0;
   prj.r0 = 0.0;
   for (j = 0; j < 10; prj.p[j++] = 0.0);
   prj.p[1] = 30.0;

   /* Set reference angles for the native grid. */
   native.ref[0] =   0.0;
   native.ref[1] =   0.0;
   native.ref[2] = 180.0;
   native.flag   = 0;

   /* For the celestial grid, set the celestial coordinates of the reference
    * point of Bonne's projection (which is at the intersection of the native
    * equator and prime meridian) and the native longitude of the celestial
    * pole.  These correspond to FITS keywords CRVAL1, CRVAL2, and LONGPOLE.
    */
   celestial.ref[0] = 150.0;
   celestial.ref[1] = -30.0;
   celestial.ref[2] = 150.0;
   celestial.flag   = 0;

   /* PGPLOT initialization. */
   strcpy(text, "/xwindow");
   pgbeg(0, text, 1, 1);

   /* Define pen colours. */
   pgscr(0, 0.00, 0.00, 0.00);
   pgscr(1, 1.00, 1.00, 0.00);
   pgscr(2, 1.00, 1.00, 1.00);
   pgscr(3, 0.80, 0.50, 0.50);
   pgscr(4, 0.50, 0.50, 0.80);
   pgscr(5, 0.80, 0.80, 0.80);
   pgscr(6, 0.80, 0.50, 0.50);
   pgscr(7, 0.50, 0.50, 0.80);
   pgscr(8, 0.30, 0.50, 0.30);

   /* Define PGPLOT viewport. */
   pgenv(-180.0, 180.0, -90.0, 130.0, 1, -2);

   /* Write a descriptive title. */
   sprintf(text, "%s projection - 15 degree graticule", pcode);
   printf("%s\n", text);
   pgtext(-180.0, -100.0, text);

   sprintf(text, "centered on celestial coordinates (%6.2lf,%6.2lf)",
      celestial.ref[0], celestial.ref[1]);
   printf("%s\n", text);
   pgtext (-180.0, -110.0, text);

   sprintf(text, "with north celestial pole at native longitude%7.2lf",
      celestial.ref[2]);
   printf("%s\n", text);
   pgtext(-180.0, -120.0, text);


   /* Draw the native coordinate grid faintly in the background. */
   pgsci(8);

   /* Draw native meridians of longitude. */
   for (ilng = -180; ilng <= 180; ilng += 15) {
      lng = (double)ilng;
      if (ilng == -180) lng = -179.99;

      j = 0;
      for (ilat = 90; ilat >= -90; ilat--) {
         lat = (double)ilat;

         if (wcsfwd(pcode, lng, lat, &native, &prj, &x, &y)) continue;

         xr[j] = -x;
         yr[j] =  y;
         j++;
      }

      pgline(j, xr, yr);
   }

   /* Draw native parallels of latitude. */
   for (ilat = 90; ilat >= -90; ilat -= 15) {
      lat = (double)ilat;

      j = 0;
      for (ilng = -180; ilng <= 180; ilng++) {
         lng = (double)ilng;
         if (ilng == -180) lng = -179.99;

         if (wcsfwd(pcode, lng, lat, &native, &prj, &x, &y)) continue;

         xr[j] = -x;
         yr[j] =  y;
         j++;
      }

      pgline(j, xr, yr);
   }


   /* Draw a colour-coded celestial coordinate grid. */
   ci = 1;

   /* Draw celestial meridians of longitude. */
   for (ilng = -180; ilng <= 180; ilng += 15) {
      lng = (double)ilng;

      if (++ci > 7) ci = 2;
      pgsci(ilng?ci:1);

      j = 0;
      for (ilat = 90; ilat >= -90; ilat--) {
         lat = (double)ilat;

         if (wcsfwd(pcode, lng, lat, &celestial, &prj, &x, &y)) continue;

         /* Test for discontinuities. */
         if (j > 0) {
            if (fabs(x+xr[j-1]) > 3.0 || fabs(y-yr[j-1]) > 3.0) {
               if (j > 1) pgline(j, xr, yr);
               j = 0;
            }
         }

         xr[j] = -x;
         yr[j] =  y;
         j++;
      }

      pgline(j, xr, yr);
   }

   /* Draw celestial parallels of latitude. */
   ci = 1;
   for (ilat = 90; ilat >= -90; ilat -= 15) {
      lat = (double)ilat;

      if (++ci > 7) ci = 2;
      pgsci(ilat?ci:1);

      j = 0;
      for (ilng = -180; ilng <= 180; ilng++) {
         lng = (double)ilng;

         if (wcsfwd(pcode, lng, lat, &celestial, &prj, &x, &y)) continue;

         /* Test for discontinuities. */
         if (j > 0) {
            if (fabs(x+xr[j-1]) > 3.0 || fabs(y-yr[j-1]) > 3.0) {
               if (j > 1) pgline(j, xr, yr);
               j = 0;
            }
         }

         xr[j] = -x;
         yr[j] =  y;
         j++;
      }

      pgline(j, xr, yr);
   }

   pgend();

   return 0;
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: twcs.c,v $ $Date: 2003/04/15 20:47:00 $ $Revision: 1.1.1.1 $ $Name:  $"};
