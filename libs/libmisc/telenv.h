extern FILE *telfopen (char *name, char *how);
extern int telopen (char *name, int flags, ...);
extern void telfixpath (char *new, char *old);
extern int telOELog(char *progname);
extern char *timestamp (time_t t);
extern void daemonLog (char *fmt, ...);

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: telenv.h,v $ $Date: 2003/04/15 20:48:18 $ $Revision: 1.1.1.1 $ $Name:  $
 */
