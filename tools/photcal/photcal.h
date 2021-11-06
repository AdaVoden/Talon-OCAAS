extern int verbose;

extern int scanDir (char dir[], double jd, double djd, PStdStar *sp, int nsp);
extern int solve_V0_kp (PStdStar *sp, int nsp, double Bkpp, double Vkpp,
    double Rkpp, double Ikpp, double V0[], double V0err[], double kp[],
    double kperr[]);

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: photcal.h,v $ $Date: 2003/04/15 20:48:35 $ $Revision: 1.1.1.1 $ $Name:  $
 */
