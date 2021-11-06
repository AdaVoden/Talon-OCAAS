/*==========================================================================*/
/* Apogee CCD Control API Internal Data Structures                          */
/*                                                                          */
/* revision   date       by                description                      */
/* -------- --------   ----     ------------------------------------------  */
/*   1.00   12/22/96    jmh     Created new API library files               */
/*                                                                          */
/*==========================================================================*/

#include "apccd.h"
#include "apdata.h"

#ifndef _apglob
#define _apglob

extern CAMDATA  camdata[MAXCAMS];   /* control data for CCD controllers     */
extern USHORT   cam_current;        /* current active camera                */
extern USHORT   cam_count;          /* number of currently open cameras     */

#endif

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: apglob.h,v $ $Date: 2003/04/15 20:48:02 $ $Revision: 1.1.1.1 $ $Name:  $
 */
