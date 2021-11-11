#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <Xm/Xm.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>

#include "configfile.h"
#include "xtools.h"
#include "telenv.h"

#include "mksch.h"

#define	PFR	100

String fallbacks[] = {
    "*fontList: -*-lucidatypewriter-bold-r-*-*-12-*-*-*-m-*-*-*",
    "*XmTextField.fontList: -*-fixed-bold-r-normal-*-14-*-*-*-c-*-*-*",
    "*XmFrame.XmLabel.fontList: -*-times-bold-i-*-*-15-*-*-*-*-*-*-*",
    "*background: wheat",
    "*foreground: #11f",
    "*highlightThickness: 0",
    "*spacing: 2",
    "*marginHeight: 2",
    "*marginWidth: 2",
    "*topMargin: 2",
    "*bottomMargin: 2",
    "*XmTextField.columns: 13",
    NULL
};

KV kv[] = {
    /* 0 */
    {"Title", "Title string for this schedule"},
    {"Observer", "Name of each project participant"},
    {"ID", "Tag for schedule and image file names"},
    {"Comments", "Additional commentary"},
    {"Name", "Name of object being imaged"},

    /* 5 */
    {"J2000 RA", "If Name is not in a catalog, J2000 RA as H:M:S"},
    {"Dec", "If Name is not in a catalog, J2000 Dec as D:M:S"},
    {"Filter(s)", "List of filters, separated with commas, e.g., B,V,R,I"},
    {"Time(s)", "Durations for each filter, seconds, e.g., 120,60,60,20"},
    {"Binning", "Pixel binning, Horizontal,Vertical, e.g., 1,1"},

    /* 10 */
    {"Subimage", "Sub frame, LeftX,TopY,Width,Height, e.g., 0,0,512,512"},
    {"Sched Dir", "Directory in which to create .sch file"},
    {"Image Dir", "Directory in which to create resulting images"},
    {"Tolerance", "Max LST or HA Start tolerance allowed, as H:M:S"},
    {"Compress", "H-Compression scale; 0 = none; 1 = max lossless"},

    /* 15 */
    {"HA Start", "HA Start constraint, if desired, as H:M:S"},
    {"LST Start", "LST Start toleratnce, if desired, as H:M:S"},
    {"Priority", "Sceduling priority, smaller values are better"},
    {"N Sets", "Total times to repeat camera set"},
    {"Block Gap", "Interval between each complete set, as H:M:S"},

    /* 20 */
    {"N Blocks", "Number of complete sets"},
};

int nkv = XtNumber(kv);

Widget msg_w;

static Widget addPrompt (Widget pf_w, Widget top_w, KV *kvp);
static Widget addFrame (Widget f_w, Widget top_w, char *title, Widget *ff_wp);
static void bottomControls (Widget p, Widget top_w);
static void getDefaults(void);

static double LSTDELTADEF;
static int COMPRESSH;
static int DEFBIN;
static int DEFIMW;
static int DEFIMH;

void
mkGui()
{
	Widget f_w, fr_w;
	Widget pf_w;
	Widget hf_w;
	Widget ff_w;

	f_w = XtVaCreateManagedWidget ("MF", xmFormWidgetClass, toplevel_w,
	    XmNverticalSpacing, 10,
	    XmNhorizontalSpacing, 10,
	    NULL);

	fr_w = addFrame (f_w, NULL, "Project", &ff_w);

	    pf_w = addPrompt (ff_w, NULL, &kv[0]);
	    pf_w = addPrompt (ff_w, pf_w, &kv[1]);
	    pf_w = addPrompt (ff_w, pf_w, &kv[2]);
	    pf_w = addPrompt (ff_w, pf_w, &kv[3]);

	fr_w = addFrame (f_w, fr_w, "Source", &ff_w);

	    pf_w = addPrompt (ff_w, NULL, &kv[4]);

	    hf_w = XtVaCreateManagedWidget ("HF", xmFormWidgetClass, ff_w,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, pf_w,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 50,
		XmNmarginWidth, 0,
		NULL);

	    (void) addPrompt (hf_w, NULL, &kv[5]);

	    hf_w = XtVaCreateManagedWidget ("HF", xmFormWidgetClass, ff_w,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, pf_w,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 50,
		XmNrightAttachment, XmATTACH_FORM,
		XmNmarginWidth, 0,
		NULL);

	    pf_w = addPrompt (hf_w, NULL, &kv[6]);

	fr_w = addFrame (f_w, fr_w, "Camera", &ff_w);

	    hf_w = XtVaCreateManagedWidget ("HF", xmFormWidgetClass, ff_w,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 50,
		XmNmarginWidth, 0,
		NULL);

	    pf_w = addPrompt (hf_w, NULL, &kv[7]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[8]);

	    hf_w = XtVaCreateManagedWidget ("HF", xmFormWidgetClass, ff_w,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 50,
		XmNrightAttachment, XmATTACH_FORM,
		XmNmarginWidth, 0,
		NULL);

	    pf_w = addPrompt (hf_w, pf_w, &kv[9]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[10]);

	fr_w = addFrame (f_w, fr_w, "Control", &ff_w);

	    pf_w = addPrompt (ff_w, NULL, &kv[11]);
	    pf_w = addPrompt (ff_w, pf_w, &kv[12]);

	    hf_w = XtVaCreateManagedWidget ("HF", xmFormWidgetClass, ff_w,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, pf_w,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 50,
		XmNmarginWidth, 0,
		NULL);

	    pf_w = addPrompt (hf_w, pf_w, &kv[13]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[14]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[15]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[16]);

	    hf_w = XtVaCreateManagedWidget ("HF", xmFormWidgetClass, ff_w,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, hf_w,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 50,
		XmNrightAttachment, XmATTACH_FORM,
		XmNmarginWidth, 0,
		NULL);

	    pf_w = addPrompt (hf_w, pf_w, &kv[17]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[18]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[20]);
	    pf_w = addPrompt (hf_w, pf_w, &kv[19]);

	bottomControls (f_w, fr_w);
}

void
initGui()
{
	char buf[1024];

	telfixpath (buf, "user/schedin");
	wtprintf (kv[11].w, "%s", buf);
	telfixpath (buf, "user/images");
	wtprintf (kv[12].w, "%s", buf);

	getDefaults();

	wtprintf (kv[0].w, "<Your title here>");
	wtprintf (kv[1].w, "<Your name here>");
	wtprintf (kv[2].w, "<Your ID here>");
	wtprintf (kv[3].w, "<Your comment here>");
	wtprintf (kv[4].w, "<Your source here>");
	wtprintf (kv[7].w, "C");
	wtprintf (kv[8].w, "60");
	wtprintf (kv[9].w, "%d,%d", DEFBIN, DEFBIN);
	wtprintf (kv[10].w, "0,0,%d,%d", DEFIMW, DEFIMH);
	wtprintf (kv[13].w, "%g", LSTDELTADEF);
	wtprintf (kv[14].w, "%d", COMPRESSH);
	wtprintf (kv[17].w, "100");
	wtprintf (kv[18].w, "1");
}

static Widget
addPrompt (Widget pf_w, Widget top_w, KV *kvp)
{
	Widget f_w;
	Widget p_w;
	Widget tf_w;

	f_w = XtVaCreateManagedWidget ("SF", xmFormWidgetClass, pf_w,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);
	if (top_w)
	    XtVaSetValues (f_w,
	    	XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_w,
		NULL);
	else
	    XtVaSetValues (f_w, XmNtopAttachment, XmATTACH_FORM, NULL);

	p_w = XtVaCreateManagedWidget ("P", xmLabelWidgetClass, f_w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNalignment, XmALIGNMENT_BEGINNING,
	    NULL);
	wlprintf (p_w, "%s:", kvp->prompt);

	tf_w = XtVaCreateManagedWidget ("TF", xmTextFieldWidgetClass, f_w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, PFR,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);

	if (kvp->tip)
	    wtip (tf_w, kvp->tip);

	kvp->w = tf_w;

	return (f_w);
}

static Widget
addFrame (Widget f_w, Widget top_w, char *title, Widget *ff_wp)
{
	Widget ff_w, fr_w, title_w;

	fr_w = XtVaCreateManagedWidget ("F", xmFrameWidgetClass, f_w,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);
	if (top_w)
	    XtVaSetValues (fr_w,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_w,
		NULL);
	else
	    XtVaSetValues (fr_w, XmNtopAttachment, XmATTACH_FORM, NULL);

	title_w = XtVaCreateManagedWidget ("FL", xmLabelWidgetClass, fr_w,
	    XmNchildType, XmFRAME_TITLE_CHILD,
	    XmNchildHorizontalAlignment, XmALIGNMENT_BEGINNING,
	    NULL);
	wlprintf (title_w, "%s", title);

	ff_w = XtVaCreateManagedWidget ("FF", xmFormWidgetClass, fr_w,
	    XmNchildType, XmFRAME_WORKAREA_CHILD,
	    NULL);

	*ff_wp = ff_w;

	return (fr_w);
}

/* ARGSUSED */
static void
ok_cb (Widget w, XtPointer client, XtPointer call)
{
	msg ("Testing...");
	if (testSched() == 0)
	    writeSched();
}

/* ARGSUSED */
static void
test_cb (Widget w, XtPointer client, XtPointer call)
{
	msg ("Testing...");
	(void) testSched ();
}


/* ARGSUSED */
static void
quit_cb (Widget w, XtPointer client, XtPointer call)
{
	exit(0);
}


static void
bottomControls (Widget p, Widget top_w)
{
	Widget f_w;
	Widget pb_w;

	msg_w = XtVaCreateManagedWidget ("RDF", xmLabelWidgetClass, p,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, top_w,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
	wlprintf (msg_w, "Welcome");

	f_w = XtVaCreateManagedWidget ("RDF", xmFormWidgetClass, p,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, msg_w,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNfractionBase, 10,
	    NULL);

	pb_w = XtVaCreateManagedWidget ("RDF", xmPushButtonWidgetClass, f_w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 10,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 3,
	    NULL);
	wlprintf (pb_w, "Save");
	XtAddCallback (pb_w, XmNactivateCallback, ok_cb, NULL);
	wtip(pb_w,"Write a new schedule to first unused file name for this ID");

	pb_w = XtVaCreateManagedWidget ("RDF", xmPushButtonWidgetClass, f_w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 10,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 4,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 6,
	    NULL);
	wlprintf (pb_w, "Test");
	XtAddCallback (pb_w, XmNactivateCallback, test_cb, NULL);
	wtip (pb_w, "Test this schedule without saving it");

	pb_w = XtVaCreateManagedWidget ("RDF", xmPushButtonWidgetClass, f_w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 10,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 7,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 9,
	    NULL);
	wlprintf (pb_w, "Quit");
	XtAddCallback (pb_w, XmNactivateCallback, quit_cb, NULL);
	wtip (pb_w, "Exit this program");
}

static void
getDefaults()
{ 
#define NTCFG  (sizeof(tcfg)/sizeof(tcfg[0]))
	static char tcfn[] = "archive/config/telsched.cfg";
	static CfgEntry tcfg[] = {
	    {"LSTDELTADEF",	CFG_DBL, &LSTDELTADEF},
	    {"COMPRESS",	CFG_INT, &COMPRESSH},
	    {"DEFBIN",		CFG_INT, &DEFBIN},
	    {"DEFIMW",		CFG_INT, &DEFIMW},
	    {"DEFIMH",		CFG_INT, &DEFIMH},
	};
	int n;

	/* read file */
	n = readCfgFile (0, tcfn, tcfg, NTCFG);
	if (n != NTCFG) {
	    cfgFileError (tcfn, n, NULL, tcfg, NTCFG);
	    exit(1);
	}
}


