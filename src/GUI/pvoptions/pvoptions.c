/****************************************************************\

	============================
	PVOPTIONS
	
	A simple GUI interface to allow easy setting of options for 1-Meter PV camera
	
	S. Ohmert August 31, 2001
	(C) 2001 Torus Technologies, Inc.  All Rights Reserved
	============================
	
	CVS History:
		$Id: pvoptions.c,v 1.1.1.1 2003/04/15 20:47:03 lostairman Exp $
		
		$Log: pvoptions.c,v $
		Revision 1.1.1.1  2003/04/15 20:47:03  lostairman
		Initial Import
		
		Revision 1.3  2002/12/14 10:31:43  steve
		added pv options for half meter
		
		Revision 1.2  2002/12/13 18:21:40  steve
		new version
		
		Revision 1.1  2001/09/03 15:29:37  steve
		created program to set pixelvision 1-m options via GUI
		
	
\****************************************************************/	

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <Xm/Xm.h>

#include "telenv.h"
#include "xtools.h"

#include "pvoptions.h"

#include "buildcfg.h"

char version[] = "2.00";
#if HALFM
char myclass[] = "PV Options -- 1/2 Meter";
#endif
#if ONEM
char myclass[] = "PV Options -- 1 Meter";
#endif
Widget toplevel_w;
XtAppContext or_app;

static void setTitle(void);

int
main (int ac, char *av[])
{
	/* default log */
	telOELog (av[0]);

	/* connect */
	toplevel_w = XtVaAppInitialize(&or_app,myclass,NULL,0,&ac,av,fallbacks,
	    XmNiconName, myclass, 
	    NULL);

	/* set title */
	setTitle();

	/* build */
	mkGui();
	initGui();

        /* go */
        XtRealizeWidget(toplevel_w);
        XtAppMainLoop(or_app);

        printf ("XtAppMainLoop returned :-)\n");
        return (1);
}

static void
setTitle()
{
	char buf[1024];

	(void) sprintf (buf, "%s -- Version %s", myclass, version);
	XtVaSetValues (toplevel_w, XmNtitle, buf, NULL);
}

/* For CVS Only -- Do Not Edit */
static char *cvsid[2] = {(char *)cvsid, "@(#) $Id: pvoptions.c,v 1.1.1.1 2003/04/15 20:47:03 lostairman Exp $"};
