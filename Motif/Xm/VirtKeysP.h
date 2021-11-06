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
/* $XConsortium: VirtKeysP.h /main/10 1995/07/13 18:21:10 drk $ */
/* (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmVirtKeysP_h
#define _XmVirtKeysP_h

#include <Xm/XmP.h>
#include <Xm/VirtKeys.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XmKEYCODE_TAG_SIZE 32

typedef struct _XmDefaultBindingStringRec {
    String	vendorName;
    String	defaults;
} XmDefaultBindingStringRec, *XmDefaultBindingString;

typedef	struct _XmVirtualKeysymRec {
    String		name;
    KeySym		keysym;
} XmVirtualKeysymRec, *XmVirtualKeysym;

/* For converting a Virtual keysym to a real keysym. */
typedef struct _XmVKeyBindingRec
{
  KeySym	keysym;
  Modifiers	modifiers;
  KeySym	virtkey;
} XmVKeyBindingRec, *XmVKeyBinding;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmVirtKeysP_h */
