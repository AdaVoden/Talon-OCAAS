#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <Xm/Xm.h>

#include "xtools.h"

#include "mksch.h"

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

