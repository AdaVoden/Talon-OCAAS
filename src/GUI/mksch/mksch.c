/* A simple GUI interface to create a .sch file */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <Xm/Xm.h>

#include "telenv.h"
#include "xtools.h"

#include "mksch.h"

char version[] = "1.01";

/* 23 Dec 97: begin work
 * 23 Feb 98: more work
 *  4 Aug 98: first useful version
 */

char myclass[] = "MkSch";
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

	(void) sprintf (buf, "Mksch -- Version %s", version);
	XtVaSetValues (toplevel_w, XmNtitle, buf, NULL);
}

