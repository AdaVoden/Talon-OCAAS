#ifndef PROJ
#define PROJ

#include "wcstrig.h"

struct prjprm {
   int flag;
   int n;
   double r0;
   double p[10];
   double w[10];
};

#if __STDC__
   int azpset(struct prjprm *);
   int azpfwd(double, double, struct prjprm *, double *, double *);
   int azprev(double, double, struct prjprm *, double *, double *);
   int tanset(struct prjprm *);
   int tanfwd(double, double, struct prjprm *, double *, double *);
   int tanrev(double, double, struct prjprm *, double *, double *);
   int sinset(struct prjprm *);
   int sinfwd(double, double, struct prjprm *, double *, double *);
   int sinrev(double, double, struct prjprm *, double *, double *);
   int stgset(struct prjprm *);
   int stgfwd(double, double, struct prjprm *, double *, double *);
   int stgrev(double, double, struct prjprm *, double *, double *);
   int arcset(struct prjprm *);
   int arcfwd(double, double, struct prjprm *, double *, double *);
   int arcrev(double, double, struct prjprm *, double *, double *);
   int zpnset(struct prjprm *);
   int zpnfwd(double, double, struct prjprm *, double *, double *);
   int zpnrev(double, double, struct prjprm *, double *, double *);
   int zeaset(struct prjprm *);
   int zeafwd(double, double, struct prjprm *, double *, double *);
   int zearev(double, double, struct prjprm *, double *, double *);
   int airset(struct prjprm *);
   int airfwd(double, double, struct prjprm *, double *, double *);
   int airrev(double, double, struct prjprm *, double *, double *);
   int cypset(struct prjprm *);
   int cypfwd(double, double, struct prjprm *, double *, double *);
   int cyprev(double, double, struct prjprm *, double *, double *);
   int carset(struct prjprm *);
   int carfwd(double, double, struct prjprm *, double *, double *);
   int carrev(double, double, struct prjprm *, double *, double *);
   int merset(struct prjprm *);
   int merfwd(double, double, struct prjprm *, double *, double *);
   int merrev(double, double, struct prjprm *, double *, double *);
   int ceaset(struct prjprm *);
   int ceafwd(double, double, struct prjprm *, double *, double *);
   int cearev(double, double, struct prjprm *, double *, double *);
   int copset(struct prjprm *);
   int copfwd(double, double, struct prjprm *, double *, double *);
   int coprev(double, double, struct prjprm *, double *, double *);
   int codset(struct prjprm *);
   int codfwd(double, double, struct prjprm *, double *, double *);
   int codrev(double, double, struct prjprm *, double *, double *);
   int coeset(struct prjprm *);
   int coefwd(double, double, struct prjprm *, double *, double *);
   int coerev(double, double, struct prjprm *, double *, double *);
   int cooset(struct prjprm *);
   int coofwd(double, double, struct prjprm *, double *, double *);
   int coorev(double, double, struct prjprm *, double *, double *);
   int bonset(struct prjprm *);
   int bonfwd(double, double, struct prjprm *, double *, double *);
   int bonrev(double, double, struct prjprm *, double *, double *);
   int pcoset(struct prjprm *);
   int pcofwd(double, double, struct prjprm *, double *, double *);
   int pcorev(double, double, struct prjprm *, double *, double *);
   int glsset(struct prjprm *);
   int glsfwd(double, double, struct prjprm *, double *, double *);
   int glsrev(double, double, struct prjprm *, double *, double *);
   int parset(struct prjprm *);
   int parfwd(double, double, struct prjprm *, double *, double *);
   int parrev(double, double, struct prjprm *, double *, double *);
   int aitset(struct prjprm *);
   int aitfwd(double, double, struct prjprm *, double *, double *);
   int aitrev(double, double, struct prjprm *, double *, double *);
   int molset(struct prjprm *);
   int molfwd(double, double, struct prjprm *, double *, double *);
   int molrev(double, double, struct prjprm *, double *, double *);
   int cscset(struct prjprm *);
   int cscfwd(double, double, struct prjprm *, double *, double *);
   int cscrev(double, double, struct prjprm *, double *, double *);
   int qscset(struct prjprm *);
   int qscfwd(double, double, struct prjprm *, double *, double *);
   int qscrev(double, double, struct prjprm *, double *, double *);
   int tscset(struct prjprm *);
   int tscfwd(double, double, struct prjprm *, double *, double *);
   int tscrev(double, double, struct prjprm *, double *, double *);
#else /* not __STDC__ */
   int azpset(), azpfwd(), azprev();
   int tanset(), tanfwd(), tanrev();
   int sinset(), sinfwd(), sinrev();
   int stgset(), stgfwd(), stgrev();
   int arcset(), arcfwd(), arcrev();
   int zpnset(), zpnfwd(), zpnrev();
   int zeaset(), zeafwd(), zearev();
   int airset(), airfwd(), airrev();
   int cypset(), cypfwd(), cyprev();
   int carset(), carfwd(), carrev();
   int merset(), merfwd(), merrev();
   int ceaset(), ceafwd(), cearev();
   int copset(), copfwd(), coprev();
   int codset(), codfwd(), codrev();
   int coeset(), coefwd(), coerev();
   int cooset(), coofwd(), coorev();
   int bonset(), bonfwd(), bonrev();
   int pcoset(), pcofwd(), pcorev();
   int glsset(), glsfwd(), glsrev();
   int parset(), parfwd(), parrev();
   int aitset(), aitfwd(), aitrev();
   int molset(), molfwd(), molrev();
   int cscset(), cscfwd(), cscrev();
   int qscset(), qscfwd(), qscrev();
   int tscset(), tscfwd(), tscrev();
#endif /* __STDC__ */

#define PI 3.141592653589793238462643
#define D2R PI/180.0
#define R2D 180.0/PI
#define SQRT2 1.4142135623730950488
#define SQRT2INV 1.0/SQRT2

#define PRJSET 137

#endif /* PROJ */
