#ifndef WCS
#define WCS

#include "proj.h"

struct wcsprm {
   int flag;
   double ref[3];
   double euler[6];

#if __STDC__
   int (*prjfwd)(double, double, struct prjprm *, double *, double *);
   int (*prjrev)(double, double, struct prjprm *, double *, double *);
#else
   int (*prjfwd)();
   int (*prjrev)();
#endif
};

#define WCSSET 137

#endif /* WCS */

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: wcs.h,v $ $Date: 2003/04/15 20:47:00 $ $Revision: 1.1.1.1 $ $Name:  $
 */
