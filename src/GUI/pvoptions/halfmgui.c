/* GUI dialog box for setting common PixelVision 1/2-M config options easily */
/* 1/2M Version 12/13/2002 */

#include "buildcfg.h"
#if HALFM

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

#include "pvoptions.h"
#include "rwcfg.h"

#define ALIGN_BOTH 	0
#define ALIGN_LEFT 	1
#define ALIGN_RIGHT 2

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

enum ids {
		    RETURN_0,RETURN_1,RETURN_2,RETURN_3,RETURN_4,
		    PROCESSING, PROC_PRESERVE,PROC_CORR,PROC_WCS,PROC_FWHM,PROC_COMP,
			PROC_RETURN};

KV kv[] = {
    /* 0 */
   	{"Center (E1/E2)", "Returns the center, splitting both chips",{1,RETURN_0}},
   	{"East Center (E1)", "Returns section east of the center",{0,RETURN_1}},
   	{"West Center (E2)", "Returns section west of center",{0,RETURN_2}},
   	{"Full E1 chip", "Returns the entire E1 chip",{0,RETURN_3}},
   	{"Full E2 chip","Returns the entire E2 chip",{0,RETURN_4}},
	/* 5 */
   	{"Enable post-processing", "Perform any post-processing on images",{1,PROCESSING}},
   	{"Preserve Center", "Keep return sections on disk, otherwise discard",{0,PROC_PRESERVE}},
   	{"Image Correction", "Perform bias/thermal/flat correction on images",{1,PROC_CORR}},
   	{"WCS", "Solve for WCS position and add to FITS header",{1,PROC_WCS}},
   	{"FWHM", "Calculate FHWM values and add to FITS header",{1,PROC_FWHM}},
   	{"Compression", "Convert to lossless h-compress FITS format",{0,PROC_COMP}},
    /* 10 */
   	{"Return to Camera", "Return results of processing to auto-listen mode Camera program",{0,PROC_RETURN}},
    /* 11 */
};

int nkv = XtNumber(kv);

EntryRec pvcamCfgEntry[] = {
	{"$ReturnChoice"},
	{"$enablePostProcessing"},
	{""} // terminator
};

EntryRec pipelineCfgEntry[] = {
	{"$preserveCenter"},
	{"$doCorrections"},
	{"$doWCS"},
	{"$doFWHM"},
	{"$imageCompression"},
	{"$resultToCamera"},
	{""} // terminator
};


Widget msg_w;
Widget saveButton_w;

static Widget addToggle (Widget pf_w, Widget top_w, KV *kvp);
static Widget addFrame (Widget f_w, Widget top_w, char *title, Widget *ff_wp, int align);
static void bottomControls (Widget p, Widget top_w);
static void getDefaults(void);
static void saveConfig(void);

void
mkGui()
{
	Widget f_w, fr_w;
	Widget pf_w;
	Widget ff_w;

	f_w = XtVaCreateManagedWidget ("MF", xmFormWidgetClass, toplevel_w,
	    XmNverticalSpacing, 10,
	    XmNhorizontalSpacing, 10,
	    NULL);
	
	fr_w = addFrame (f_w, NULL, "Return Choices", &ff_w, ALIGN_LEFT);

	    pf_w = addToggle (ff_w, NULL, &kv[RETURN_0]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_1]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_2]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_3]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_4]);

	fr_w = addFrame (f_w, NULL, "Processing options", &ff_w, ALIGN_RIGHT);

	    pf_w = addToggle (ff_w, NULL, &kv[PROCESSING]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_PRESERVE]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_CORR]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_WCS]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_FWHM]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_COMP]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_RETURN]);
	
	bottomControls (f_w, fr_w);
}

void
initGui()
{
	getDefaults();

}

/* set truth of int pointed to by client, unless NULL */
static void
toggleCB (Widget w, XtPointer client, XtPointer call)
{
	int i;
	int *ip = (int *)client;
	int oldval;
	
	if (ip)
	{
		oldval = *ip;
	    *ip = XmToggleButtonGetState (w);
		
	    // return choice is a radio button	
		if(ip[1] >= RETURN_0 && ip[1] <= RETURN_4) {
			
			if(!ip[0]) {
				XmToggleButtonSetState(w,TRUE,TRUE);	// don't deselect current setting
				return;
			}
			
			for(i=RETURN_0; i<=RETURN_4; i++) {
				if(i != ip[1]) {
					XmToggleButtonSetState(kv[i].w,FALSE,FALSE);
					kv[i].stateid[0] = FALSE;
				}
			}
		}
		
		switch(ip[1]) {
								
			case PROCESSING:				
				for(i=PROCESSING+1; i<=PROC_RETURN; i++) {
					XtSetSensitive(kv[i].w,ip[0]);				
				}
				break;
		}
	
		if(oldval != ip[0]) {
			XtSetSensitive(saveButton_w,TRUE);
		}
	}	
}


static Widget addToggle(Widget pf_w, Widget top_w, KV *kvp)
{
	Arg args[20];
	Widget f_w,w;
	int n;

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
		
	n = 0;	
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNset, kvp->stateid[0]); n++;
	w = XmCreateToggleButton (f_w, kvp->prompt, args, n);
	XtAddCallback (w, XmNvalueChangedCallback, toggleCB,
						(XtPointer)(&kvp->stateid[0]));
	XtManageChild (w);

	if (kvp->tip)
	    wtip (w, kvp->tip);

	kvp->w = w;	
	
	return (w);
}

static Widget
addFrame (Widget f_w, Widget top_w, char *title, Widget *ff_wp, int align)
{
	Widget ff_w, fr_w, title_w;

	if(align == ALIGN_LEFT)
	{
		fr_w = XtVaCreateManagedWidget ("F", xmFrameWidgetClass, f_w,
		    XmNleftAttachment, XmATTACH_FORM,
//	    	XmNrightAttachment, XmATTACH_FORM,
	    	XmNleftOffset, 15,
		    NULL);
	}
	else if(align == ALIGN_RIGHT)
	{
		fr_w = XtVaCreateManagedWidget ("F", xmFrameWidgetClass, f_w,
//		    XmNleftAttachment, XmATTACH_FORM,
	    	XmNrightAttachment, XmATTACH_FORM,
	    	XmNrightOffset, 15,
		    NULL);
	}
	else
	{
		fr_w = XtVaCreateManagedWidget ("F", xmFrameWidgetClass, f_w,
		    XmNleftAttachment, XmATTACH_FORM,
	    	XmNrightAttachment, XmATTACH_FORM,
		    NULL);
	}
	
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
	saveConfig();
}


/* ARGSUSED */
static void
quit_cb (Widget w, XtPointer client, XtPointer call)
{
	msg ("Goodbye!");
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
	wlprintf (msg_w, "   _____________________________________________________   ");

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
	wtip(pb_w,"Write values to config file and activate");
	saveButton_w = pb_w;
	
	pb_w = XtVaCreateManagedWidget ("RDF", xmPushButtonWidgetClass, f_w,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 10,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 7,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 9,
	    NULL);
	wlprintf (pb_w, "Exit");
	XtAddCallback (pb_w, XmNactivateCallback, quit_cb, NULL);
	wtip (pb_w, "Quit this program");
}

static void
getDefaults()
{
	/* read the values from the config files into our dialog */
	
	char * p;
	int i,val;
	
	// First read from the main config file
	if(0 == readValuesFromConfig("archive/config/pvcam.cfg",pvcamCfgEntry))
	{		
		// Get return choice
		p = getFileValue(pvcamCfgEntry,"$ReturnChoice");
		if(p) {
			val = atoi(p);
			for(i=RETURN_0; i<=RETURN_4; i++) {				
				XmToggleButtonSetState(kv[i].w,FALSE,FALSE);
			}
			XmToggleButtonSetState(kv[RETURN_0+val].w,TRUE,TRUE);
		}
		
		// Do misc options
		p = getFileValue(pvcamCfgEntry,"$enablePostProcessing");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROCESSING].stateid[0] = val;
			XmToggleButtonSetState(kv[PROCESSING].w,val,val);
		}				
			
	}
	
	// Now read from the pipeline config file
	if(0 == readValuesFromConfig("archive/config/pipeline.cfg",pipelineCfgEntry))
	{
		p = getFileValue(pipelineCfgEntry,"$preserveCenter");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_PRESERVE].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_PRESERVE].w,val,val);
		}
		p = getFileValue(pipelineCfgEntry,"$doCorrections");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_CORR].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_CORR].w,val,val);
		}
		p = getFileValue(pipelineCfgEntry,"$doWCS");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_WCS].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_WCS].w,val,val);
		}
		p = getFileValue(pipelineCfgEntry,"$doFWHM");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_FWHM].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_FWHM].w,val,val);
		}
		p = getFileValue(pipelineCfgEntry,"$imageCompression");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_COMP].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_COMP].w,val,val);
		}
		p = getFileValue(pipelineCfgEntry,"$resultToCamera");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_RETURN].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_RETURN].w,val,val);
		}
	}		

	XtSetSensitive(saveButton_w,FALSE);				
}

/* write our dialog setttings out to config files */
static void saveConfig()
{
	char * p;
	int i;
	int rt;
	
	// start by populating the entry recs with our dialog values
	p = getOurValue(pvcamCfgEntry, "$ReturnChoice");
	if(p) {
		for(i=RETURN_0; i<=RETURN_4; i++) {
			if(kv[i].stateid[0]) break;
		}
		if(i > RETURN_4) i = RETURN_0;
		sprintf(p,"%d",i-RETURN_0);
	}
	p = getOurValue(pvcamCfgEntry, "$enablePostProcessing");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROCESSING].w));
	}
	
	p = getOurValue(pipelineCfgEntry, "$preserveCenter");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_PRESERVE].w));
	}
	p = getOurValue(pipelineCfgEntry, "$doCorrections");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_CORR].w));
	}
	p = getOurValue(pipelineCfgEntry, "$doWCS");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_WCS].w));
	}
	p = getOurValue(pipelineCfgEntry, "$doFWHM");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_FWHM].w));
	}
	p = getOurValue(pipelineCfgEntry, "$imageCompression");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_COMP].w));
	}
	p = getOurValue(pipelineCfgEntry, "$resultToCamera");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_RETURN].w));
	}

	// now write the file out
	rt = writeNewCfgFile("archive/config/pvcam.cfg",pvcamCfgEntry);
	if(!rt) rt = writeNewCfgFile("archive/config/pipeline.cfg",pipelineCfgEntry);
	else
	{
		msg("Error saving pvcam.cfg");
		return;
	}

	if(!rt) XtSetSensitive(saveButton_w,FALSE);				
	else
	{
		msg("Error saving pipeline.cfg");
		return;
	}
			
}

#endif // halfm


/* For CVS Only -- Do Not Edit */
static char *cvsid[2] = {(char *)cvsid, "@(#) $Id: halfmgui.c,v 1.1.1.1 2003/04/15 20:47:03 lostairman Exp $"};
