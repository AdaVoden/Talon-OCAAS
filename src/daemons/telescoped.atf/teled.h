/* dio96.c */
extern void dio96_ctrl (int regn, unsigned char ctrl);
extern void dio96_getallbits (unsigned char bits[12]);
extern void dio96_setbit (int n);
extern void dio96_clrbit (int n);

/* dio24.c */
extern void dio24_ctrl (int regn, unsigned char ctrl);
extern void dio24_getallbits (unsigned char bits[3]);
extern void dio24_setbit (int n);
extern void dio24_clrbit (int n);

/* domectrl.c */
extern int haveDome(void);
extern void initDome(void);
extern void autoDome(int whether);
extern void resetDome(void);
extern void readDomeAz (double *azp);
extern void stopDome(void);
extern void rotateDomeNE(void);
extern void rotateDomeNW(void);
extern void domeParallax (double alt, double az, double *azcor);

/* filter.c */
extern int filt_have(void);
extern int filt_reset(void);
extern int filt_starthome(void);
extern int filt_chkhome(void);
extern int filt_set (char *name);
extern int filt_readraw(void);

/* focus.c */
extern int focus_have(void);
extern int focus_reset(void);
extern int focus_starthome(void);
extern int focus_chkhome(void);
extern int focus_set (char *npstr);
extern int focus_readraw(void);

/* mountcor.c */
extern void init_mount_cor(void);
extern void tel_mount_cor (double ha, double dec, double *dhap, double *ddecp);

/* pc39.c */
extern int pc39_w (char *fmt, ...);
extern int pc39_wr (char rbuf[], int rbuflen, char *fmt, ...);
extern int pc39_reset (void);

/* shutter.c */
extern int haveShutter(void);
extern void initShutter(void);
extern int openShutter(void);
extern int closeShutter(void);
extern void stopShutter(void);

/* telctrl.c */
extern int tel_reset(int hwtoo);
extern int tel_readraw(void);
extern int tel_startFindingLimits(void);
extern int tel_FindingLimits(void);
extern int tel_starthome(void);
extern int tel_chkhome(void);
extern int tel_slew (int first);
extern int tel_hunt (int ms, int first);
extern int tel_track (int ms);
extern int tel_sane_progress (int init);
extern void tel_allstop(void);
extern void tel_stop(void);
extern void tel_ap2as (double Mjd, double *rap, double *decp);

/* telescoped.c */
extern int teled_trace;
extern int pc39_trace;
extern int nohw;
extern void tdlog (char *fmt, ...);
extern void die(void);

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: teled.h,v $ $Date: 2003/04/15 20:47:56 $ $Revision: 1.1.1.1 $ $Name:  $
 */
