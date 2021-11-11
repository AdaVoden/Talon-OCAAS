*=======================================================================
*
*   WCSLIB - an implementation of the FITS WCS proposal.
*   Copyright (C) 1995, Mark Calabretta
*
*   This library is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2 of the
*   License, or (at your option) any later version.
*
*   This library is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Library General Public License for more details.
*
*   You should have received a copy of the GNU Library General Public
*   License along with this library; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Correspondence concerning WCSLIB may be directed to:
*      Internet email: mcalabre@atnf.csiro.au
*      Postal address: Dr. Mark Calabretta,
*                      Australia Telescope National Facility,
*                      P.O. Box 76,
*                      Epping, NSW, 2121,
*                      AUSTRALIA
*
*=======================================================================
*
* FORTRAN interfaces to FORTRAN PGPLOT routines used by the WCSLIB test
* programs.  These are provided in order to define the length of
* character variables.
*
* $Id: tpgf.f,v 1.1.1.1 2003/04/15 20:46:59 lostairman Exp $
*-----------------------------------------------------------------------

      SUBROUTINE PGBEG2 (UNIT, FILE, NXSUB, NYSUB)
      INTEGER   NXSUB, NYSUB, UNIT
      CHARACTER FILE*80

      CALL PGBEG (UNIT, FILE, NXSUB, NYSUB)

      RETURN
      END

*-----------------------------------------------------------------------

      SUBROUTINE PGTXT2 (X, Y, TEXT)
      REAL      X, Y
      CHARACTER TEXT*80

      CALL PGTEXT (X, Y, TEXT)

      RETURN
      END
