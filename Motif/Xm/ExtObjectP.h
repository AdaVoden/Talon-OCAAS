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
/*   $XConsortium: ExtObjectP.h /main/11 1995/07/14 10:32:48 drk $ */
/* (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/* (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/* (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/* (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef _XmExtObjectP_h
#define _XmExtObjectP_h

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  XmCACHE_EXTENSION = 1,		XmDESKTOP_EXTENSION,
  XmSHELL_EXTENSION,			XmPROTOCOL_EXTENSION,
  XmDEFAULT_EXTENSION
};

#ifndef XmIsExtObject
#define XmIsExtObject(w)	XtIsSubclass(w, xmExtObjectClass)
#endif /* XmIsExtObject */

#define XmLOGICAL_PARENT_RESOURCE	(0x80 << sizeof(Cardinal))

/* Class record constants */

typedef struct _XmExtRec *XmExtObject;
typedef struct _XmExtClassRec *XmExtObjectClass;

externalref WidgetClass xmExtObjectClass;

/* Class Extension definitions */

typedef struct _XmExtClassPart {
  XmSyntheticResource *syn_resources;   
  int		       num_syn_resources;   
  XtPointer	       extension;
} XmExtClassPart, *XmExtClassPartPtr;

typedef struct _XmExtClassRec {
  ObjectClassPart object_class;
  XmExtClassPart  ext_class;
} XmExtClassRec;

typedef struct {
  Widget	logicalParent;
  unsigned char	extensionType;
} XmExtPart, *XmExtPartPtr;

externalref XmExtClassRec xmExtClassRec;

typedef struct _XmExtRec {
  ObjectPart object;
  XmExtPart  ext;
} XmExtRec;


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmExtObjectP_h */
