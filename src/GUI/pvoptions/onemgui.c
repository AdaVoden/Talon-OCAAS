/* GUI dialog box for setting common PixelVision 1-M config options easily */
/* S. Ohmert 9/02/01 */
/* Version 2.0 12/05/2002 */

#include "buildcfg.h"
#if ONEM

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

enum ids {ENABLE_B,ENABLE_C,ENABLE_D,
		    RETURN_0,RETURN_1,RETURN_2,RETURN_3,RETURN_4,RETURN_5,
            RETURN_B1,RETURN_B2,RETURN_B3,RETURN_B4,
            RETURN_C1,RETURN_C2,RETURN_C3,
            RETURN_D1,RETURN_D2,RETURN_D3,
		    PROCESSING, PROC_PRESERVE,PROC_CORR,PROC_WCS,PROC_FWHM,PROC_COMP,
			MISC_FAKEFLAT};

KV kv[] = {
    /* 0 */
   	{"Southern Section (B)", "Enables Group 'B', along image bottom",{1,ENABLE_B}},
    {"Northwest Section (C)", "Enables Group 'C', right side top/center",{1,ENABLE_C}},
   	{"Northeast Section (D)", "Enables Group 'D', left side top/center",{1,ENABLE_D}},
    /* 3 */
   	{"West Center (C1)", "Returns the center, just west of pointing origin",{1,RETURN_0}},
   	{"East Center (D3)", "Returns the center, just east of pointing origin",{0,RETURN_1}},
   	{"Northwest Center (B2)", "Returns northwest of center",{0,RETURN_2}},
   	{"Northeast Center (B3)", "Returns northeast of center",{0,RETURN_3}},
   	{"Southwest Center (C3)", "Returns upper left corner of C3 chip",{0,RETURN_4}},
   	{"Southeast Center (D1)", "Returns upper right corner of D1 chip",{0,RETURN_5}},
   	{"Chip B1", "Returns the entire B1 chip",{0,RETURN_B1}},    	
   	{"Chip B2", "Returns the entire B2 chip",{0,RETURN_B2}},    	
   	{"Chip B3", "Returns the entire B3 chip",{0,RETURN_B3}},    	
   	{"Chip B4", "Returns the entire B4 chip",{0,RETURN_B4}},    	
   	{"Chip C1", "Returns the entire C1 chip",{0,RETURN_C1}},    	
   	{"Chip C2", "Returns the entire C2 chip",{0,RETURN_C2}},    	
   	{"Chip C3", "Returns the entire C3 chip",{0,RETURN_C3}},    	
   	{"Chip D1", "Returns the entire D1 chip",{0,RETURN_D1}},    	
   	{"Chip D2", "Returns the entire D2 chip",{0,RETURN_D2}},    	
   	{"Chip D3", "Returns the entire D3 chip",{0,RETURN_D3}},    	
	/* 19 */
   	{"Enable post-processing", "Perform any post-processing on images",{1,PROCESSING}},
   	{"Preserve Center", "Keep return sections on disk, otherwise discard",{0,PROC_PRESERVE}},
   	{"Image Correction", "Perform bias/thermal/flat correction on images",{1,PROC_CORR}},
   	{"WCS", "Solve for WCS position and add to FITS header",{1,PROC_WCS}},
   	{"FWHM", "Calculate FHWM values and add to FITS header",{1,PROC_FWHM}},
   	{"Compression", "Convert to lossless h-compress FITS format",{0,PROC_COMP}},
    /* 25 */
   	{"Fake Flat", "Create a 'fake' flat when performing image calibration steps",{0,MISC_FAKEFLAT}},
    /* 27 */
};

int nkv = XtNumber(kv);

EntryRec pvc1mCfgEntry[] = {
	{"@GroupEnable"},
	{"$ReturnChoice"},
	{"$FakeFlats"},
//	{"$useSkyMonitor"},
//	{"$storeSkyMonitorImgs"},
	{"$enablePostProcessing"},
	{""} // terminator
};

EntryRec pipeln1mCfgEntry[] = {
	{"$preserveCenter"},
	{"$doCorrections"},
	{"$doWCS"},
	{"$doFWHM"},
	{"$imageCompression"},
//	{"$resultToCamera"},
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
	
	fr_w = addFrame (f_w, NULL, "Enable Region", &ff_w, ALIGN_LEFT);

	    pf_w = addToggle (ff_w, NULL, &kv[ENABLE_B]);
	    pf_w = addToggle (ff_w, pf_w, &kv[ENABLE_C]);
	    pf_w = addToggle (ff_w, pf_w, &kv[ENABLE_D]);

	fr_w = addFrame (f_w, fr_w, "Processing options", &ff_w, ALIGN_LEFT);

	    pf_w = addToggle (ff_w, NULL, &kv[PROCESSING]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_PRESERVE]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_CORR]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_WCS]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_FWHM]);
	    pf_w = addToggle (ff_w, pf_w, &kv[PROC_COMP]);
	
	fr_w = addFrame (f_w, fr_w, "Other Options", &ff_w, ALIGN_LEFT);

	    pf_w = addToggle (ff_w, pf_w, &kv[MISC_FAKEFLAT]);

	fr_w = addFrame (f_w, NULL, "Return Choices", &ff_w, ALIGN_RIGHT);

	    pf_w = addToggle (ff_w, NULL, &kv[RETURN_0]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_1]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_2]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_3]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_4]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_5]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_B1]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_B2]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_B3]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_B4]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_C1]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_C2]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_C3]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_D1]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_D2]);
	    pf_w = addToggle (ff_w, pf_w, &kv[RETURN_D3]);
	
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
		if(ip[1] >= RETURN_0 && ip[1] <= RETURN_D3) {
			
			if(!ip[0]) {
				XmToggleButtonSetState(w,TRUE,TRUE);	// don't deselect current setting
				return;
			}
			
			for(i=RETURN_0; i<=RETURN_D3; i++) {
				if(i != ip[1]) {
					XmToggleButtonSetState(kv[i].w,FALSE,FALSE);
					kv[i].stateid[0] = FALSE;
				}
			}
		}
		
		switch(ip[1]) {
								
			case PROCESSING:				
				for(i=PROCESSING+1; i<MISC_FAKEFLAT; i++) {
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
	if(0 == readValuesFromConfig("archive/config/pvc1m.cfg",pvc1mCfgEntry))
	{
		// read and parse group enable
		p = getFileValue(pvc1mCfgEntry,"@GroupEnable");
		if(p) {
			i = 0;
			while(*p && i<4) {
				if(*p >= '0' && *p <= '9') {
                    if(i) { // BECAUSE WE REMOVED ENABLE_A
    					val = (*p != '0');
	    				kv[ENABLE_B+i-1].stateid[0] = val;
		    			XmToggleButtonSetState(kv[ENABLE_B+i-1].w,val,val);
                    }
					i++;					
				}
				p++;
			}
		}
		
		// Get return choice
		p = getFileValue(pvc1mCfgEntry,"$ReturnChoice");
		if(p) {
			val = atoi(p);
			for(i=RETURN_0; i<=RETURN_D3; i++) {				
				XmToggleButtonSetState(kv[i].w,FALSE,FALSE);
			}
			XmToggleButtonSetState(kv[RETURN_0+val].w,TRUE,TRUE);
		}
		
		// Do misc options
		p = getFileValue(pvc1mCfgEntry,"$FakeFlats");
		if(p) {
			val = (atoi(p) != 0);
			kv[MISC_FAKEFLAT].stateid[0] = val;
			XmToggleButtonSetState(kv[MISC_FAKEFLAT].w,val,val);
		}
/*
		p = getFileValue(pvc1mCfgEntry,"$useSkyMonitor");
		if(p) {
			val = (atoi(p) != 0);
			kv[MISC_USESKY].stateid[0] = val;
			XmToggleButtonSetState(kv[MISC_USESKY].w,val,val);
		}
		p = getFileValue(pvc1mCfgEntry,"$storeSkyMonitorImgs");
		if(p) {
			val = (atoi(p) != 0);
			kv[MISC_STORESKY].stateid[0] = val;
			XmToggleButtonSetState(kv[MISC_STORESKY].w,val,val);
		}				
*/
		p = getFileValue(pvc1mCfgEntry,"$enablePostProcessing");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROCESSING].stateid[0] = val;
			XmToggleButtonSetState(kv[PROCESSING].w,val,val);
		}				
			
	}
	
	// Now read from the pipeline config file
	if(0 == readValuesFromConfig("archive/config/pipeln1m.cfg",pipeln1mCfgEntry))
	{
		p = getFileValue(pipeln1mCfgEntry,"$preserveCenter");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_PRESERVE].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_PRESERVE].w,val,val);
		}
		p = getFileValue(pipeln1mCfgEntry,"$doCorrections");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_CORR].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_CORR].w,val,val);
		}
		p = getFileValue(pipeln1mCfgEntry,"$doWCS");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_WCS].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_WCS].w,val,val);
		}
		p = getFileValue(pipeln1mCfgEntry,"$doFWHM");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_FWHM].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_FWHM].w,val,val);
		}
		p = getFileValue(pipeln1mCfgEntry,"$imageCompression");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_COMP].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_COMP].w,val,val);
		}
/*
		p = getFileValue(pipeln1mCfgEntry,"$resultToCamera");
		if(p) {
			val = (atoi(p) != 0);
			kv[PROC_RETURN].stateid[0] = val;
			XmToggleButtonSetState(kv[PROC_RETURN].w,val,val);
		}
*/		
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
	p = getOurValue(pvc1mCfgEntry, "@GroupEnable");
	if(p){
		sprintf(p,"(%d,%d,%d,%d)",
			0,/*kv[ENABLE_A].stateid[0],*/
			kv[ENABLE_B].stateid[0],
			kv[ENABLE_C].stateid[0],
			kv[ENABLE_D].stateid[0]);
	}
	p = getOurValue(pvc1mCfgEntry, "$ReturnChoice");
	if(p) {
		for(i=RETURN_0; i<=RETURN_D3; i++) {
			if(kv[i].stateid[0]) break;
		}
		if(i > RETURN_D3) i = RETURN_0;
		sprintf(p,"%d",i-RETURN_0);
	}
	p = getOurValue(pvc1mCfgEntry, "$FakeFlats");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[MISC_FAKEFLAT].w));
	}
/*
	p = getOurValue(pvc1mCfgEntry, "$useSkyMonitor");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[MISC_USESKY].w));
	}
	p = getOurValue(pvc1mCfgEntry, "$storeSkyMonitorImgs");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[MISC_STORESKY].w));
	}
*/
	p = getOurValue(pvc1mCfgEntry, "$enablePostProcessing");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROCESSING].w));
	}
	
	p = getOurValue(pipeln1mCfgEntry, "$preserveCenter");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_PRESERVE].w));
	}
	p = getOurValue(pipeln1mCfgEntry, "$doCorrections");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_CORR].w));
	}
	p = getOurValue(pipeln1mCfgEntry, "$doWCS");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_WCS].w));
	}
	p = getOurValue(pipeln1mCfgEntry, "$doFWHM");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_FWHM].w));
	}
	p = getOurValue(pipeln1mCfgEntry, "$imageCompression");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_COMP].w));
	}
/*
	p = getOurValue(pipeln1mCfgEntry, "$resultToCamera");
	if(p) {
		sprintf(p,"%d",XmToggleButtonGetState(kv[PROC_RETURN].w));
	}
*/	
	// now write the file out
	rt = writeNewCfgFile("archive/config/pvc1m.cfg",pvc1mCfgEntry);
	if(!rt) rt = writeNewCfgFile("archive/config/pipeln1m.cfg",pipeln1mCfgEntry);
	else
	{
		msg("Error saving pvc1m.cfg");
		return;
	}

	if(!rt) XtSetSensitive(saveButton_w,FALSE);				
	else
	{
		msg("Error saving pipeln1m.cfg");
		return;
	}
			
}

#endif // ONEM

/* For CVS Only -- Do Not Edit */
static char *cvsid[2] = {(char *)cvsid, "@(#) $Id: onemgui.c,v 1.1.1.1 2003/04/15 20:47:03 lostairman Exp $"};
