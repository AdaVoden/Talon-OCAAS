/* ids for fifos.
 * N.B. see fifos[] in telescoped.c
 */
typedef enum {
    Tel_Id, Filter_Id, Focus_Id, Dome_Id, Lights_Id, Power_Id
} FifoId;

/* atf */
extern int atf_have(void);
extern void atf_cfg(void);
extern void atf_readraw (int *xp, int *yp);

/* axes.c */
extern int axis_home (MotorInfo *mip, FifoId fid, int first);
extern int axis_limits (MotorInfo *mip, FifoId fid, int first);
extern void setSpeedDefaults (MotorInfo *mip);
extern int axisLimitCheck (MotorInfo *mip, char msgbuf[]);
extern int axisMotionCheck (MotorInfo *mip, char msgbuf[]);

/* dio.c */
extern void dio96_ctrl (int regn, unsigned char ctrl);
extern void dio96_getallbits (unsigned char bits[12]);
extern void dio96_setbit (int n);
extern void dio96_clrbit (int n);

/* dome.c */
extern void dome_msg (char *msg);

/* fifoio.c */
extern void fifoWrite (FifoId f, int code, char *fmt, ...);
extern void init_fifos(void);
extern void chk_fifos(void);
extern void close_fifos(void);

/* filter.c */
extern void filter_msg (char *msg);
extern FilterInfo *findFilter (char name);

/* focus.c */
extern void focus_msg (char *msg);

/* lights.c */
extern void lights_msg (char *msg);

/* mountcor.c */
extern void init_mount_cor(void);
extern void tel_mount_cor (double ha, double dec, double *dhap, double *ddecp);

/* pc39.c */
extern int pc39_w (char *fmt, ...);
extern int pc39_wr (char rbuf[], int rbuflen, char *fmt, ...);
extern int pc39_reset (void);

/* power */
extern double STOWALT, STOWAZ;
extern int chkPowerfail (void);
extern void power_msg (char *msg);

/* tel.c */
extern void tel_msg (char *msg);

/* telescoped.c */
extern TelStatShm *telstatshmp;
extern int nohw;
extern char tscfn[];
extern char tdcfn[];
extern char hcfn[];
extern char ocfn[];
extern char icfn[];
extern char dcfn[];
extern void init_cfg(void);
extern void allstop(void);
extern void tdlog (char *fmt, ...);
extern void die(void);

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: teled.h,v $ $Date: 2003/04/15 20:47:58 $ $Revision: 1.1.1.1 $ $Name:  $
 */
