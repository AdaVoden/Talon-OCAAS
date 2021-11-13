/* interfaces to the device-dependent files */

extern int bflag;
extern int rflag;

extern int PBUpdate (char *tty, WxStats *wp, double *tp, double *pp);
extern int DUpdate (char *tty, WxStats *wp, double *tp, double *pp);
extern int RWUpdate (char *tty, WxStats *wp, double *tp, double *pp);
extern int URLUpdate (char *url, WxStats *wp, double *tp, double *pp);

extern char DT_TPL[];
extern char WS_TPL[];
extern char WD_TPL[];
extern char WN_TPL[];
extern char HM_TPL[];
extern char RN_TPL[];
extern char PR_TPL[];
extern char TM_TPL[];
