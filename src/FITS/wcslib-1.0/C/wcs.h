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
