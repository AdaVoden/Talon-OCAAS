/* this daemon listens to several FIFOS for generic telescope, dome and
 * instrument commands and manipulates the lower-level hardware accordingly.
 *
 * The commands are usually a command/response pair on any one
 * FIFO pair. The Response may be "Ok" if the command was successful (and will
 * not be generated until it indeed is) or perhaps other responses such as
 * "Error", "Abort" or "Interrupted". All responses are preceeded with a
 * unique digit as well, with 0 always associated with "Ok". The fifo names
 * all end with ".in" and ".out" which are with respect to us, the server.
 *
 * N.B. it is up to the clients to wait for a response; if they just send 
 *   another command the previous response will be dropped and will not get a
 *   response. Other fifos in pending states will be sent an Interrupted
 *   message.
 *
 * as an aid to debugging, -d means not to run as a deamon. in this mode, 
 *   telescoped listens to and exits when stdin yields EOF.
 *
 * FIFO pairs:
 *   Ctlr	misc control commands, such as to Reset, Limit, Home, Stop.
 *   Slew	slew and stop; format: "HA:%g Dec:%g" or "Alt:%g Az:%g", rads.
 *   Track	slew and track; xephem edb format
 *   Filter	desired filter, first char of name as setup in filter.cfg.
 *   Focus	desired focus offset -- arbitrary linear units :-Z
 *   Dome	rotate dome to given az, "Az:%g", rads +E of N.
 *   Shutter	"Open" or "Close" shutter
 *
 * v2.2 8/27/97	add raw to paddle. shorten some pc39 commands.
 * v2.1 5/7/97  start final round of changes for IRO
 * v2.0	2/24/97  active tracking for alt/az scopes, new fifo msg formats.
 * v1.10 11/4/96 switch to more generic FIFO pairs.
 * v1.9 10/31/96 change mount correction to use a mesh
 * v1.8 9/13/96 change focus so it moves relative
 * v1.7 3/10	add shutter control
 * v1.6 3/7	add focus control
 * v1.5 2/26	add dome control once it moved to T axis of pc34.
 * v1.4 2/4/94	add support for the Joystick stream
 * v1.3	1/9/94	Reset now also re-reads the config file.
 * v1.2 12/15/93 init i{ha,dec} to current encoder pos.
 * v1.1 12/13/93 change STOP stream to TelCTRL and implement RESET command.
 * v1.0	12/7/93	first production version; changed to nohw runtime option.
 * v0.5 12/2/93 verified all math.
 * v0.4 11/27/93 change stream pipe, shm names; add NOHW #define
 * v0.3 11/8/93	 Change response logic.
 * v0.2 10/29/93 Use stream pipes, not fifos.
 * v0.1	10/28/93 First draft: Elwood C. Downey
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"
#include "configfile.h"
#include "db.h"
#include "misc.h"
#include "telstatshm.h"
#include "running.h"
#include "cliserv.h"
#include "teled.h"


#define	MAXLINE		4096	/* max message from a fifo */

TelStatShm *telstatshmp;	/* shared telescope info */
int teled_trace;		/* >0 for trace */
int nohw;			/* 0 normal; 1 no hardware */

/* info about a fifo connection */
typedef struct {
    char *name;		/* fifo name; also env name to override default*/
    void (*fp)();	/* function to call to process input from this fifo */
    int tel_alert;	/* commands from this fifo effect the basic mount */
    int wantreply;	/* set when this fifo is expecting a reply */
    int fdin;		/* fifo descriptor to us, once opened */
    int fdout;		/* fifo descriptor from us, once opened */
} FifoInfo;

static void ctrl_fifo (FifoInfo *fip, char *msg);
static void slew_fifo (FifoInfo *fip, char *msg);
static void track_fifo (FifoInfo *fip, char *msg);
static void filter_fifo (FifoInfo *fip, char *msg);
static void focus_fifo (FifoInfo *fip, char *msg);
static void paddle_fifo (FifoInfo *fip, char *msg);
static void dome_fifo (FifoInfo *fip, char *msg);
static void shutter_fifo (FifoInfo *fip, char *msg);

/* quick indices into the fifo[] array, below.
 * N.B. must be in same order as the fifo array, below
 */
typedef enum {
    Dome_F=0, Shutter_F, Ctrl_F, Slew_F, Track_F, Paddle_F, Filter_F, Focus_F,
    N_F
} FifoName;

/* array of info about each fifo pair we deal with.
 * N.B. must be in same order as the FifoName enum, above
 */
static FifoInfo fifo[N_F] = {
    {"Dome",		dome_fifo,	0},
    {"Shutter",		shutter_fifo,	0},
    {"Ctrl",		ctrl_fifo,	1},
    {"Slew",		slew_fifo,	1},
    {"Track",		track_fifo,	1},
    {"Paddle",		paddle_fifo,	1},
    {"Filter",		filter_fifo,	0},
    {"Focus",		focus_fifo,	0},
};

static long shutter_to;	/* shutter command completes now, or 0 if not using it*/

/* following values are set from the config files */
static int POLL_PERIOD;		/* min time between updates, ms */
static double DOMEHYS;		/* close enough when dome hunting, rads */
static double SHUTTERAZ;	/* dome az at shutter slip rings, rads EofN*/
static double SHUTTERAZHYS;	/* dome az slip rings hysterisis, rads */
static double MASTER_TO;	/* master timeout, days */

static double taz;		/* target dome azimuth, rads +E of N */
static char *progname;		/* me */
static int hwreset;		/* set with -r flag */
static double mjd_to;		/* mjd when current op times out; 0 for never */

/* common reply messages */
static char ok_msg[] = "0 Ok";
static char format_msg[] = "1 Bad Command Format";
static char interrupted_msg[] = "2 Interrupted";
static char err_msg[] = "3 Execution Error";
static char limits_msg[] = "4 Hard limit";
static char nodev_msg[] = "5 Not installed";
static char stall_msg[] = "6 Hardware is stalled";
static char notnow_msg[] = "7 Can't do that now";
static char to_msg[] = "8 Operation timed out";

static void usage (char *progname);
static void init_all(void);
static void init_shm(void);
static void init_cfg(void);
static void daemonize(void);
static int open_fifos(void);
static void on_term(int fake);
static void periodic_update(void);
static void reply (FifoInfo *fip, char *msg);
static void alert_all (FifoInfo *fip, char *msg);
static void alert_tel (char *msg);

int
main (ac, av)
int ac;
char *av[];
{
	int nodaemon = 0;
	FifoInfo *fip;
	int maxfdp1;
	char *str;

	/* save name */
	progname = av[0];

	/* crack arguments */
	for (av++; --ac > 0 && *(str = *av) == '-'; av++) {
	    char c;
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'd':	/* don't become a deamon */
		    nodaemon++;
		    break;
		case 'h':	/* no hardware */
		    nohw = 1;
		    break;
		case 'r':	/* hardware reset */
		    hwreset = 1;
		    break;
		case 'v':	/* more verbosity */
		    config_trace++; 
		    teled_trace++;
		    if (teled_trace > 1)
			pc39_trace++;
		    break;
		default: usage(progname); break;
		}
	}

	/* now there are ac remaining args starting at av[0] */
	if (ac > 0)
	    usage(progname);

	/* run as a new, unencumbered process from here on (unless -d).
	 * we (the parent) exits.
	 */
	if (!nodaemon)
	    daemonize();

	/* only ever one.
	 * N.B. must follow daemonize() in case it forks.
	 */ 
	if (lock_running(progname) < 0) {
	    tdlog ("%s: Already running or can't tell", progname);
	    exit(2);
	}

	/* init all subsystems once */
	init_all();

	/* open all the fifos.
	 * save the max file descriptor, plus 1, for use with select().
	 */
	maxfdp1 = open_fifos();
	if (maxfdp1 < 0)
	    exit (1);
	maxfdp1++;

	/* infinite loop processing commands from the fifo and/or
	 * timing out to manage work that needs periodic attention.
	 */
	while (1) {
	    struct timeval tv;
	    fd_set rfdset;
	    int i, s;

	    /* initialize the select read mask of fds for which we care.
	     * listen to stdin for eof too if we are not a daemon.
	     */
	    FD_ZERO (&rfdset);
	    for (i = 0; i < N_F; i++)
		if (fifo[i].fdin >= 0) /* set to -1 if had to close */
		    FD_SET (fifo[i].fdin, &rfdset);
	    if (nodaemon)
		FD_SET (0, &rfdset);

	    /* set up the polling delay */
	    tv.tv_sec = POLL_PERIOD/1000;
	    tv.tv_usec = 1000*(POLL_PERIOD%1000);

	    /* call select, waiting for commands or timeout */
	    s = select (maxfdp1, &rfdset, NULL, NULL, &tv);
	    if (s < 0) {
		if (errno == EINTR)
		    continue;	/* repeat on signal */
		perror ("select error");
		sleep(2);	/* don't want to just die */
	    }

	    /* exit if we are not running as a daemon and we get eof on stdin */
	    if (nodaemon && FD_ISSET (0, &rfdset)) {
		char buf[128];
		if (read (0, buf, sizeof(buf)) == 0)
		    die();
	    }

	    /* do whatever might be needed periodically */
	    periodic_update();

	    /* read and process any one message arriving on the fifo(s)..
	     * N.B. just do one so periodic_update() runs fresh for each.
	     */
	    for (fip = fifo; fip < &fifo[N_F]; fip++) {
		if (FD_ISSET (fip->fdin, &rfdset)) {
		    char msg[MAXLINE];
		    int n = read (fip->fdin, msg, sizeof(msg)-1);
		    if (n <= 0) {
			if (n < 0)
			    perror (fip->name);
			else
			    tdlog ("unexpected EOF from %s", fip->name);

			/* close the channel */
			close (fip->fdin);
			fip->fdin = -1;
			close (fip->fdout);
			fip->fdout = -1;
		    }

		    msg[n] = '\0';		/* insure it's a C string */
		    if (teled_trace)
			tdlog ("from %s: %.*s", fip->name, n, msg);
		    (*fip->fp) (fip, msg);	/* call this fifos's handler */

		    break;
		}
	    }
	}
}

/* print a usage message and exit */
static void
usage (progname)
char *progname;
{
	fprintf (stderr, "%s: [-dhv]\n", progname);
	fprintf (stderr, "\td: does not make into a daemon.\n");
	fprintf (stderr, "\th: fake the hardware.\n");
	fprintf (stderr, "\tr: force an initial full hardware reset.\n");
	fprintf (stderr, "\tv: more makes more verbose.\n");
	exit (1);
}

/* print a message to stderr */
void
tdlog (char *fmt, ...)
{
	char datebuf[128], msgbuf[1024];
	va_list ap;
	time_t t;
	struct tm *tmp;

	time (&t);
	tmp = gmtime (&t);
	sprintf (datebuf, "%02d/%02d/%02d %02d:%02d:%02d: ", tmp->tm_mon+1,
	    tmp->tm_mday, tmp->tm_year, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

	va_start (ap, fmt);

	fputs (datebuf, stderr);
	vsprintf (msgbuf, fmt, ap);
	fputs (msgbuf, stderr);
	if (msgbuf[strlen(msgbuf)-1] != '\n')
	    fputs ("\n", stderr);

	va_end (ap);
}

/* initialize all the various subsystems.
 * N.B. call this only once.
 */
static void
init_all()
{
	/* connect to the telstatshm segment */
	init_shm();

	/* read the config files */
	init_cfg();

	/* initialize the telescope subsystem  */
	if (tel_reset(hwreset) < 0)
	    exit (1);

	/* initialize the filter wheel */
	if (filt_reset() < 0)
	    exit (1);

	/* initialize the focus motor */
	if (focus_reset() < 0)
	    exit (1);

	/* init the dome.
	 * make sure dome is stopped and we believe it should be where it is
	 */
	initDome();
	if (haveDome() == 0) {
	    stopDome();
	    telstatshmp->autodome = 0;
	    telstatshmp->domestatus = DS_STOPPED;
	    readDomeAz (&taz);
	    telstatshmp->dometaz = telstatshmp->domeaz = taz;
	}

	/* init the shutter control */
	if (haveShutter() == 0)
	    initShutter();

	/* connect the signal handlers */
	signal (SIGINT, on_term);
	signal (SIGTERM, on_term);
	signal (SIGHUP, on_term);
}

static void
init_shm()
{
	int shmid;
	long addr;

	shmid = shmget (TELSTATSHMKEY, sizeof(TelStatShm), 0666|IPC_CREAT);
	if (shmid < 0) {
	    perror ("shmget TELSTATSHMKEY");
	    exit (1);
	}

	addr = (long) shmat (shmid, (void *)0, 0);
	if (addr == -1) {
	    perror ("shmat TELSTATSHMKEY");
	    exit (1);
	}

	telstatshmp = (TelStatShm *) addr;
}

/* read the config files for our variables here, then read for each
 * specialized suubsystem.
 */
static void
init_cfg()
{
	double tmp;

	/* read local circumstances for telstatshmp->now */
	setConfigFile ("archive/config/telsched.cfg", 1);
	readDVal ("LONGITUDE", &tmp);
	telstatshmp->now.n_lng = -tmp;	/* we want rads +E */
	readDVal ("LATITUDE", &tmp);
	telstatshmp->now.n_lat = tmp;	/* we want rads +N */
	readDVal ("TEMPERATURE", &tmp);
	telstatshmp->now.n_temp = tmp;	/* we want degrees C */
	readDVal ("PRESSURE", &tmp);
	telstatshmp->now.n_pressure = tmp;	/* we want mB */
	readDVal ("ELEVATION", &tmp);
	telstatshmp->now.n_elev = tmp/ERAD;	/* we want earth radii */

	/* read polling rate */
	setConfigFile ("archive/config/telescoped.cfg", 1);
	readIVal ("POLL_PERIOD", &POLL_PERIOD);
	readDVal ("MASTERTO", &MASTER_TO);
	MASTER_TO /= 60.*24.0;		/* mins to days */

	/* read some dome and shutter hardware characteristics */
	setConfigFile ("archive/config/domed.cfg", 1);
	readDVal ("DOMEHYS", &DOMEHYS);
	readDVal ("SHUTTERAZ", &SHUTTERAZ);
	readDVal ("SHUTTERAZHYS", &SHUTTERAZHYS);
}

/* make a new process to serve as the telescope daemon.
 * this only returns if we are the new daemon process.
 */
static void
daemonize()
{
	switch (fork()) {
	case -1: perror ("fork"); exit (1); break;
	case 0: break;
	default: exit (0);
	}
}

/* create and attach all the fifos.
 * if all ok return the max fd we created,
 * else complain to stderr and return -1.
 */
static int
open_fifos()
{
	FifoInfo *fip;
	int maxfd = 0;

	for (fip = fifo; fip < &fifo[N_F]; fip++) {
	    char msg[1024];
	    int fd[2];

	    if (serv_conn (fip->name, fd, msg) < 0) {
		fprintf (stderr, "%s\n", msg);
		exit(1);
	    }

	    fip->fdin = fd[0];
	    fip->fdout = fd[1];

	    if (fd[0] > maxfd)
		maxfd = fd[0];
	    if (fd[1] > maxfd)
		maxfd = fd[1];
	}

	return (maxfd);
}

/* stop the telescope then exit */
void
die()
{
	FifoInfo *fip;

	if (teled_trace)
	    tdlog ("die()");

	tel_allstop();
	telstatshmp->telstatus = TS_STOPPED;

	if (telstatshmp)
	    (void) shmdt ((void *)telstatshmp);

	for (fip = fifo; fip < &fifo[N_F]; fip++) {
	    int fd[2];

	    fd[0] = fip->fdin;
	    fd[1] = fip->fdout;

	    dis_conn (fip->name, fd);
	}

	unlock_running (progname, 0);

	exit (0);
}

static void
on_term(int fake)
{
	die();
}

/* called when we receive a message from the CTRL fifo.
 */
/* ARGSUSED */
static void
ctrl_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	/* inform telescope clients who might be waiting */
	alert_tel (interrupted_msg);

	/* digest the command */
	if (strncasecmp (msg, "reset", 5) == 0) {
	    /* software reset and all stop */
	    init_cfg();
	    if (tel_reset(0) + filt_reset() + focus_reset()) {
		tel_allstop();	/* panic */
		telstatshmp->telstatus = TS_STOPPED;
		reply (fip, "9 reset error");
	    } else {
		tel_stop();	/* controlled */
		telstatshmp->telstatus = TS_STOPPED;
		reply (fip, ok_msg);
	    }
	    telstatshmp->telstatus = TS_STOPPED;

	} else if (strncasecmp (msg, "home", 4) == 0) {
	    /* full reset then reset home positions.
	     * set telstatus right away because reset can take a while.
	     */
	    init_cfg();
	    telstatshmp->telstatus = TS_HOMING;
	    if (tel_reset(1) + filt_reset() + focus_reset()) {
		tel_allstop();	/* panic */
		telstatshmp->telstatus = TS_STOPPED;
		reply (fip, "9 home reset error");
	    } else {
		telstatshmp->telstatus = TS_HOMING;
		if (tel_starthome() + filt_starthome() + focus_starthome()) {
		    tel_allstop();	/* panic */
		    telstatshmp->telstatus = TS_STOPPED;
		    reply (fip, "9 start home error");
		} else {
		    telstatshmp->telstatus = TS_HOMING;
		    mjd_to = telstatshmp->now.n_mjd + MASTER_TO;
		    fip->wantreply = 1;
		    /* no reply until all finished homing */
		}
	    }

	} else if (strncasecmp (msg, "limits", 6) == 0) {
	    /* find the limits of each axis */
	    init_cfg();
	    if (tel_reset(0) + filt_reset() + focus_reset()) {
		tel_allstop();	/* panic */
		telstatshmp->telstatus = TS_STOPPED;
		reply (fip, "9 limits reset error");
	    } else {
		if (tel_startFindingLimits()) {
		    tel_allstop();	/* panic */
		    telstatshmp->telstatus = TS_STOPPED;
		    reply (fip, "9 start limits error");
		} else {
		    telstatshmp->telstatus = TS_LIMITING;
		    mjd_to = 0.0; /* can take gosh awful long */
		    fip->wantreply = 1;
		    /* no reply until all finished finding limits */
		}
	    }

	} else {
	    /* anything else is a full stop */
	    tel_allstop();
	    telstatshmp->telstatus = TS_STOPPED;
	    reply (fip, ok_msg);
	}
}

/* called when we receive a message from the Slew fifo.
 */
static void
slew_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	Obj *op = &telstatshmp->obj;
	Now *np = &telstatshmp->now;
	double lst, ra, ha, dec, alt, az;
	char rstr[32], dstr[32], edb[1024];

	/* set ha and dec to target location.
	 * accept either ha/dec or alt/az formats.
	 */
	if (sscanf (msg, "Alt:%lf Az:%lf", &alt, &az) == 2) {
	    aa_hadec (lat, alt, az, &ha, &dec);
	} else if (sscanf (msg, "HA:%lf Dec:%lf", &ha, &dec) != 2) {
	    reply (fip, format_msg);
	    return;
	}

	/* committed to trying -- inform other clients who might be waiting */
	alert_tel (interrupted_msg);

	/* build a fictitious object at ha, dec */
	now_lst (np, &lst);
	lst = hrrad(lst);
	ra = lst - ha;
	range (&ra, 2*PI);

	/* convert to astrometric */
	tel_ap2as (J2000, &ra, &dec);

	/* easiest to just synthesize an edb entry and use crack_line */
	fs_sexa (rstr, radhr(ra), 3, 360000);
	fs_sexa (dstr, raddeg(dec), 3, 36000);
	(void) sprintf (edb, "Slew Target,f,%s,%s,0.0,2000", rstr, dstr);
        (void) db_crack_line (edb, op);	/* can't fail -- it's ours :-) */

	/* get moving */
	if (tel_slew (1) < 0) {
	    reply (fip, limits_msg);
	    tel_stop();
	    telstatshmp->telstatus = TS_STOPPED;
	} else {
	    telstatshmp->telstatus = TS_SLEWING;
	    mjd_to = telstatshmp->now.n_mjd + MASTER_TO;
	    fip->wantreply = 1;
	}
}

/* called when we receive a message from the Track fifo.
 */
static void
track_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	Obj *op = &telstatshmp->obj;
	double ra, dec, year;
	char buf[1024];
	int ns;

	/* inform other clients who might be waiting */
	alert_tel (interrupted_msg);

	/* read request, accept both old way or .edb way.
	 * if no Epoch, assume EOD/apparent
	 */
	ns = sscanf (msg, "RA:%lf Dec:%lf Epoch:%lf", &ra, &dec, &year);
	if (ns >= 2) {
	    /* fake up a fixed edb description */
	    char rstr[32], dstr[32];
	    if (ns == 2) {
		tel_ap2as (J2000, &ra, &dec);
		year = 2000.0;
	    }
	    fs_sexa (rstr, radhr(ra), 3, 36000);
	    fs_sexa (dstr, raddeg(dec), 3, 3600);
	    (void) sprintf (buf, "Track RADec,f,%s,%s,0.0,%g", rstr, dstr,year);
	    msg = buf;
	} /* else try msg in .edb format */

	/* synthesize a new obj */
	if (db_crack_line (msg, op) < 0) {
	    reply (fip, format_msg);
	    return;
	}

	/* get moving */
	if (tel_hunt (POLL_PERIOD, 1) < 0) {
	    reply (fip, limits_msg);
	    telstatshmp->telstatus = TS_STOPPED;
	    tel_stop();
	} else {
	    telstatshmp->telstatus = TS_HUNTING;
	    mjd_to = telstatshmp->now.n_mjd + MASTER_TO;
	    fip->wantreply = 1;
	}
}

/* called when we receive a message from the Paddlestick fifo.
 * actually uses the slew and hunt facilty.
 */
static void
paddle_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	Obj *op = &telstatshmp->obj;
	double a, b;

	/* update the appropriate offset fields */
	if (strncasecmp (msg, "reset", 5) == 0) {
	    telstatshmp->odra = telstatshmp->oddec = 0.0;
	    telstatshmp->odalt = telstatshmp->odaz = 0.0;
	    telstatshmp->odh = telstatshmp->odd = 0.0;
	} else if (sscanf (msg, "dX:%lf dY:%lf", &a, &b) == 2) {
	    telstatshmp->odh += a;
	    telstatshmp->odd += b;
	} else if (sscanf (msg, "dHA:%lf dDec:%lf", &a, &b) == 2) {
	    telstatshmp->odra -= a;
	    telstatshmp->oddec += b;
	} else if (sscanf (msg, "dAlt:%lf dAz:%lf", &a, &b) == 2) {
	    telstatshmp->odalt += a;
	    telstatshmp->odaz += b;
	} else {
	    reply (fip, format_msg);
	    return;
	}

	/* committed, inform other clients who might be waiting and
	 * then paddle fifo gets the response when complete.
	 */
	alert_tel (interrupted_msg);

	/* hunt or slew, depending on what we're already doing */
	switch (telstatshmp->telstatus) {
	case TS_TRACKING:	/* fallthru */

	case TS_HUNTING:
	    /* hunt for new target */
	    if (tel_hunt (POLL_PERIOD, 1) < 0)
		reply (fip, limits_msg);
	    else {
		telstatshmp->telstatus = TS_HUNTING;
		mjd_to = telstatshmp->now.n_mjd + MASTER_TO;
		fip->wantreply = 1;
	    }
	    break;

	case TS_STOPPED:
	    /* init slew relative to where we are now */
	    (void) strcpy (op->o_name, "SlewTarget");
	    op->o_type = FIXED;
	    op->f_RA = telstatshmp->J2000RA;
	    op->f_dec = telstatshmp->J2000Dec;
	    op->f_epoch = J2000;
	    if (tel_slew (1) < 0)
		reply (fip, limits_msg);
	    else {
		telstatshmp->telstatus = TS_SLEWING;
		mjd_to = telstatshmp->now.n_mjd + MASTER_TO;
		fip->wantreply = 1;
	    }
	    break;

	case TS_SLEWING:
	    /* just let standard slew seeking work */
	    break;
	
	default:
	    reply (fip, notnow_msg);
	    break;
	}
}

/* called when we receive a message from the Filter fifo.
 */
/* ARGSUSED */
static void
filter_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	int d;

	if (filt_have() < 0) {
	    reply (fip, nodev_msg);
	    return;
	}

	if (!strncasecmp (msg, "stop", 4) || !strncasecmp (msg, "reset", 5)) {
	    if (filt_reset() < 0)
		reply (fip, "9 filter reset error");
	    else
		reply (fip, ok_msg);
	    return;
	}

	/* command the new setting */
	d = filt_set (msg);	/* fills msg when error */
	if (d < 0) {
	    char buf[128];
	    sprintf (buf, "1 %s", msg);
	    reply (fip, buf);
	    return;
	}

	fip->wantreply = 1;
}

/* called when we receive a message from the Focus fifo.
 */
/* ARGSUSED */
static void
focus_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	int d;

	if (focus_have() < 0) {
	    reply (fip, nodev_msg);
	    return;
	}

	if (!strncasecmp (msg, "stop", 4) || !strncasecmp (msg, "reset", 5)) {
	    if (focus_reset() < 0)
		reply (fip, "9 focus reset error");
	    else
		reply (fip, ok_msg);
	    return;
	}

	/* command the new setting */
	d = focus_set (msg);
	if (d < 0) {
	    reply (fip, "9 focus set error");
	    return;
	}

	fip->wantreply = 1;
}

/* called when we receive a message from the Dome fifo.
 * it can be a specific Az or the commands "Auto" or "Manual".
 * anything else resets and stops.
 */
/* ARGSUSED */
static void
dome_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	double az;

	if (haveDome() < 0) {
	    reply (fip, nodev_msg);
	    return;
	}

	if (!strncasecmp (msg, "auto", 4)) {
	    autoDome(1); /* sets telstatshmp->autodome = 1 */
	    reply (fip, ok_msg);
	    return;	/* main loop will start monitoring telescope */
	} else if (sscanf (msg, "Az:%lf", &az) != 1) {
	    /* anything else other than AZ switches to manual control. 
	     * reread the config file.
	     * stop the dome and set target to current so it looks correct.
	     */
	    init_cfg();
	    resetDome();
	    telstatshmp->autodome = 0;
	    stopDome();
	    readDomeAz (&taz);
	    telstatshmp->dometaz = taz;
	    telstatshmp->domestatus = DS_STOPPED;
	    reply (fip, ok_msg);
	    return;	/* we now just listen for explicit az commands */
	}

	/* set manual mode, insure it's in the proper range and save it */
	stopDome();
	autoDome (0);	/* sets telstatshmp->autodome = 0 */
	range (&az, 2*PI);
	if (az < DOMEHYS) az = DOMEHYS;
	if (az > 2*PI-DOMEHYS) az = 2*PI-DOMEHYS;
	taz = az;

	/* set state to rotating .. main loop will start the actual move soon */
	telstatshmp->domestatus = DS_ROTATING;
	fip->wantreply = 1;
}

/* called when we receive a message from the Shutter fifo.
 * only commands are Stop Open and Close.
 * we must first move to SHUTTERAZ to get power from the slip rings.
 */
/* ARGSUSED */
static void
shutter_fifo (fip, msg)
FifoInfo *fip;
char *msg;
{
	if (haveShutter() < 0) {
	    reply (fip, nodev_msg);
	    return;
	}

	if (!strncasecmp (msg, "stop", 4)) {
	    initShutter(); 	/* stop shutter where it is now */
	    shutter_to = 0;
	    reply (fip, ok_msg);
	    return;
	}

	/* ok, now we should have either Open or Close.
	 * set shutterstate -- polling does all the work.
	 * N.B. we treat Reset like a close too.
	 */
	if (!strncasecmp (msg, "open", 4)) {
	    telstatshmp->shutterstatus = SHTR_OPENING;
	} else if (!strncasecmp(msg,"close",5) || !strncasecmp(msg,"reset",5)) {
	    telstatshmp->shutterstatus = SHTR_CLOSING;
	} else {
	    reply (fip, format_msg);
	    return;
	}

	/* head dome to az where power wipers are located */
	taz = SHUTTERAZ;

	fip->wantreply = 1;
}

/* timer expired. See what periodic work there is to do.
 * N.B. we might decide to change the telstatshmp->telstatus.
 */
static void
periodic_update()
{
	int shutteraction;
	double daz;
	double err;
	int th, oh, ih;
	int n;

	/* update current time -- might be separate process someday */
	telstatshmp->now.n_mjd = mjd_now();

	/* check for timeout */
	if (telstatshmp->telstatus != TS_STOPPED && mjd_to
					&& telstatshmp->now.n_mjd > mjd_to) {
	    alert_all (NULL, to_msg);
	    (void) tel_reset(0);
	    (void) focus_reset();
	    (void) filt_reset();
	    return;
	}

	/* update all axis info in telstatshm */
	if (filt_readraw() || focus_readraw() || tel_readraw()) {
	    alert_all(NULL, "9 raw read error");
	    (void) tel_reset(0);
	    (void) focus_reset();
	    (void) filt_reset();
	    return;
	}

	/* sanity check */
	if (tel_sane_progress (0) < 0) {
	    alert_all (NULL, stall_msg);
	    (void) tel_reset(0);
	    (void) focus_reset();
	    (void) filt_reset();
	}

	/* check on the telescope.
	 * if TS_HOMING, we wait for focus and filter here too.
	 */
	switch (telstatshmp->telstatus) {
	case TS_HOMING:
	    /* check homing progress -- done when all axes are done */
	    th = tel_chkhome();
	    oh = focus_chkhome();
	    ih = filt_chkhome();

	    if (th == 1 && oh == 1 && ih == 1) {
		alert_all (NULL, ok_msg);
		telstatshmp->telstatus = TS_STOPPED;
	    }
	    if (th < 0 || oh < 0 || ih < 0) {
		char buf[128];
		(void) sprintf (buf, "9 Home error: t=%d o=%d i=%d", th, oh,ih);
		alert_all (NULL, buf);
		telstatshmp->telstatus = TS_STOPPED;
	    }
	    break;

	case TS_LIMITING:
	    /* checking limit-seeking progress */
	    n = !tel_FindingLimits();
	    if (n) {
		alert_all (NULL, ok_msg);
		telstatshmp->telstatus = TS_STOPPED;
	    }
	    break;

	case TS_STOPPED:
	    /* we don't activity hold the current position while stopped */
	    break;

	case TS_SLEWING:
	    /* check slewing progress -- done when hit limit or target */
	    switch (tel_slew (0)) {
	    case -1: 	/* hit limit */
		alert_tel (limits_msg);
		tel_stop();
		telstatshmp->telstatus = TS_STOPPED;
		break;
	    case 0:	/* still moving */
		break;
	    case 1:	/* bingo! */
		alert_tel (ok_msg);
		tel_stop();	/* just paranoid */
		telstatshmp->telstatus = TS_STOPPED;

		/* reset offsets when hit target -- it's a feature */
		telstatshmp->odra = telstatshmp->oddec = 0.0;
		telstatshmp->odalt = telstatshmp->odaz = 0.0;
		telstatshmp->odh = telstatshmp->odd = 0.0;

		break;
	    }

	    break;

	case TS_HUNTING:
	    /* check acquisition progress -- track if find, stop if hit limit */
	    switch (tel_hunt (POLL_PERIOD, 0)) {
	    case -1: 	/* hit limit */
		alert_tel (limits_msg);
		tel_stop();
		telstatshmp->telstatus = TS_STOPPED;
		break;
	    case 0:	/* still moving */
		break;
	    case 1:	/* bingo! */
		alert_tel (ok_msg);
		(void) tel_track (POLL_PERIOD);
		telstatshmp->telstatus = TS_TRACKING;
		mjd_to = 0.0;	/* no timeout */
		break;
	    }

	    break;

	case TS_TRACKING:
	    /* actively maintain target position */
	    if (tel_track (POLL_PERIOD) < 0) {
		/* hit limit */
		alert_tel (limits_msg);
		tel_stop();
		telstatshmp->telstatus = TS_STOPPED;
	    }

	    break;

	default:
	    tdlog ("Bogus tel_state: %d", telstatshmp->telstatus);
	    break;
	}

	/* several places care whether we are trying to work with the shutter */
	shutteraction = telstatshmp->shutterstatus == SHTR_OPENING ||
				telstatshmp->shutterstatus == SHTR_CLOSING;

	/* check on dome.
	 * we temporarily ignore autodome if trying to work the shutter.
	 */
	if (haveDome() == 0) {
	    daz = taz;	    /* effects 0 err hence no auto rotate if no dome */
	    readDomeAz (&daz);  /* fills telstatshmp->domeaz */
	    if(telstatshmp->autodome && !shutteraction) {
		double alt, az;

		/* set taz based on current telescope position, corrected for
		 * parallax
		 */
		hadec_aa (telstatshmp->now.n_lat, telstatshmp->EODHA,
					    telstatshmp->EODDec, &alt, &az);
		domeParallax (alt, az, &taz);
	    }
	    telstatshmp->dometaz = taz;
	    err = taz - daz;
	    if (teled_trace > 2)
		tdlog ("taz=%g az=%g", taz, daz);
	    if ((shutteraction && fabs(err) > SHUTTERAZHYS) ||
				    (!shutteraction && fabs(err) > DOMEHYS)) {
		if (err < 0)
		    rotateDomeNW();
		else
		    rotateDomeNE();
		telstatshmp->domestatus = DS_ROTATING;
	    } else {
		if (telstatshmp->domestatus != DS_STOPPED) {
		    stopDome();
		    telstatshmp->domestatus = DS_STOPPED;
		}
		if (fifo[Dome_F].wantreply) {
		    reply (&fifo[Dome_F], ok_msg);
		    fifo[Dome_F].wantreply = 0;
		}
	    }
	}

	/* check the shutter
	 * N.B. we rely on domestatus so this must follow checking the dome.
	 */
	if (telstatshmp->domestatus == DS_STOPPED && shutteraction) {
	    if (shutter_to) {
		long now = time(0);
		if (now >= shutter_to) {
		    stopShutter(); /* sets shutterstatus to something else */
		    shutter_to = 0;
		    if (fifo[Shutter_F].wantreply) {
			reply (&fifo[Shutter_F], ok_msg);
			fifo[Shutter_F].wantreply = 0;
		    }
		}
	    } else {
		int dt;
		if (telstatshmp->shutterstatus == SHTR_OPENING)
		    dt = openShutter();		/* start to open shutter */
		else
		    dt = closeShutter();	/* start to close shutter */
		/* set the shutter timer -- periodic will timeout and send reply
		 */
		shutter_to = time(0);
		shutter_to += dt;
	    }
	}

	/* check on the filter -- HOMING is handled elsewhere */
	if (telstatshmp->telstatus != TS_HOMING && fifo[Filter_F].wantreply) {
	    if (telstatshmp->irate == 0) {
		/* position aquired when motor stops */
		reply (&fifo[Filter_F], ok_msg);
		fifo[Filter_F].wantreply = 0;
	    }
	}

	/* check on the focus -- HOMING is handled elsewhere */
	if (telstatshmp->telstatus != TS_HOMING && fifo[Focus_F].wantreply) {
	    if (telstatshmp->orate == 0) {
		/* position aquired when motor stops */
		reply (&fifo[Focus_F], ok_msg);
		fifo[Focus_F].wantreply = 0;
	    }
	}
}

/* write a message to the given fifo.
 * it's ok if no one is listening.
 */
static void
reply (fip, msg)
FifoInfo *fip;
char *msg;
{
	int l, nw;

	if (teled_trace)
	    tdlog ("reply(): %s -> %s", fip->name, msg);

	l = strlen (msg) + 1;	/* include trailing \0 */
	nw = write (fip->fdout, msg, l);
	if (nw < 0 && teled_trace)
	    perror (fip->name);
}

/* send the given message to all who are waiting, except notfip */
static void
alert_all (FifoInfo *notfip, char *msg)
{
	FifoInfo *fip;

	for (fip = fifo; fip < &fifo[N_F]; fip++) {
	    if (fip->wantreply && fip != notfip) {
		reply (fip, msg);
		fip->wantreply = 0;
	    }
	}
}

/* send the given message to all tel_alert users who are waiting */
static void
alert_tel (char *msg)
{
	FifoInfo *fip;

	for (fip = fifo; fip < &fifo[N_F]; fip++) {
	    if (fip->wantreply && fip->tel_alert) {
		reply (fip, msg);
		fip->wantreply = 0;
	    }
	}
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: telescoped.c,v $ $Date: 2003/04/15 20:47:57 $ $Revision: 1.1.1.1 $ $Name:  $"};
