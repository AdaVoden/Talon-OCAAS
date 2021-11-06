/* 
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
*/ 
/* 
 * HISTORY
 * $Log: ClipWindTI.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:51  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.1.2.1  1996/01/03  18:44:01  dbl
 * 	CR 10308: if container's icon header is set after created and realized, scroll incorrect
 * 	[1996/01/03  18:43:23  dbl]
 *
 * $EndLog$
*/ 

#ifndef _XmClipWindowTI_H
#define _XmClipWindowTI_H

#include <Xm/Xm.h>
#include <Xm/NavigatorT.h>

#ifdef __cplusplus
extern "C" {
#endif

externalref XrmQuark _XmQTclipWindow;

/* Version 0: initial release. */

typedef struct _XmClipWindowTraitRec {
  int				    version;		/* 0 */
} XmClipWindowTraitRec, *XmClipWindowTrait;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmClipWindowTI_H */
