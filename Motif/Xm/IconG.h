/* $XConsortium: IconG.h /main/5 1995/07/15 20:52:04 drk $ */
/*
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
 */
/*
 * HISTORY
 */
#ifndef _XmIconG_h
#define _XmIconG_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif



/* Class record constants */
extern	WidgetClass	xmIconGadgetClass;

typedef struct _XmIconGadgetClassRec * XmIconGadgetClass;
typedef struct _XmIconGadgetRec      * XmIconGadget;

#ifndef XmIsIconGadget
#define XmIsIconGadget(w) XtIsSubclass(w, xmIconGadgetClass)
#endif /* XmIsIconGadget */

/********    Public Function Declarations    ********/
extern Widget XmCreateIconGadget(
                        Widget parent,
                        String name,
                        ArgList arglist,
                        Cardinal argcount) ;

/********    End Public Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmIconG_h */

/* DON'T ADD ANYTHING AFTER THIS #endif */
