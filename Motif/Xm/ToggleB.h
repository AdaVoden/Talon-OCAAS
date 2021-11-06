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
/*   $XConsortium: ToggleB.h /main/12 1995/07/13 18:11:58 drk $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/***********************************************************************
 *
 * Toggle Widget
 *
 ***********************************************************************/
#ifndef _XmToggle_h
#define _XmToggle_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

externalref WidgetClass xmToggleButtonWidgetClass;

typedef struct _XmToggleButtonClassRec *XmToggleButtonWidgetClass;
typedef struct _XmToggleButtonRec      *XmToggleButtonWidget;

/*fast subclass define */
#ifndef XmIsToggleButton
#define XmIsToggleButton(w)     XtIsSubclass(w, xmToggleButtonWidgetClass)
#endif /* XmIsToggleButton */


/********    Public Function Declarations    ********/

extern Boolean XmToggleButtonGetState( 
                        Widget w) ;
extern void XmToggleButtonSetState( 
                        Widget w,
#if NeedWidePrototypes
                        int newstate,
                        int notify) ;
#else
                        Boolean newstate,
                        Boolean notify) ;
#endif /* NeedWidePrototypes */

extern Boolean
XmToggleButtonSetValue(
        Widget w,
#if NeedWidePrototypes
        int newstate,
        int notify );
#else
        XmToggleButtonState newstate,
        Boolean notify );
#endif /* NeedWidePrototypes */
extern Widget XmCreateToggleButton( 
                        Widget parent,
                        char *name,
                        Arg *arglist,
                        Cardinal argCount) ;

/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmToggle_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
