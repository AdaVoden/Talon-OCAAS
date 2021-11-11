#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <Xm/Xm.h>

#include "xtools.h"

#include "pvoptions.h"

/* write a new message to the msg area */
void
msg (char *fmt, ...)
{
	va_list ap;
	char buf[512];

	/* 0-length lines cause the label to shrink strangely */
	if (fmt[0] == '\0')
	    fmt = " ";

	va_start (ap, fmt);
	vsprintf (buf, fmt, ap);
	wlprintf (msg_w, "%s", buf);
	va_end (ap);

	XmUpdateDisplay (msg_w);
}

/* For CVS Only -- Do Not Edit */
static char *cvsid[2] = {(char *)cvsid, "@(#) $Id: msg.c,v 1.1.1.1 2003/04/15 20:47:03 lostairman Exp $"};
