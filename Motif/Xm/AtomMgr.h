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
/*   $XConsortium: AtomMgr.h /main/10 1995/07/14 10:10:31 drk $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmAtomMgr_h
#define _XmAtomMgr_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* As of X11r5 XInternAtom is cached by Xlib, so we can use it directly. */

#define XmInternAtom(display, name, only_if_exists) \
		XInternAtom(display, name, only_if_exists)
#define XmGetAtomName(display, atom) \
		XGetAtomName(display, atom)

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

/* This macro name is confusing, and of unknown benefit.
 * #define XmNameToAtom(display, atom) \
 *      XmGetAtomName(display, atom)
 */

#endif /* _XmAtomMgr_h */
