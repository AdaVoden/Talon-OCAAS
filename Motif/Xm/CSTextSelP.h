/*
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
 */
/*
 * HISTORY
 * $Log: CSTextSelP.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:50  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.1.6.1  1994/08/04  18:55:56  yak
 * 	Fixed problems with copyrights
 * 	[1994/08/04  18:39:54  yak]
 *
 * Revision 1.1.4.1  1994/03/04  22:21:32  drk
 * 	Hide _Xm routines from public view.
 * 	[1994/03/04  21:55:05  drk]
 * 
 * Revision 1.1.2.6  1993/07/23  02:47:05  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:12:13  yak]
 * 
 * Revision 1.1.2.5  1993/07/02  22:08:58  drand
 * 	Fixed UTM to not do fallback targets for Export and Clipboard.
 * 	Changed fields in convertCallbackStruct to use better more
 * 	intrinsic like names.  Fixed a few bugs in the widget.  Added
 * 	better handling for getting the selection event.
 * 	[1993/07/02  21:58:12  drand]
 * 
 * Revision 1.1.2.4  1993/07/02  17:20:34  motifdec
 * 	Removed _XmCSTextSecondaryConvert
 * 	[1993/07/02  17:12:34  motifdec]
 * 
 * Revision 1.1.2.3  1993/06/25  18:46:34  motifdec
 * 	Remove QuickCut and QuickCopy decls.
 * 	[1993/06/25  18:43:04  motifdec]
 * 
 * Revision 1.1.2.2  1993/06/16  20:19:10  motifdec
 * 	June 16 code drop from digital
 * 	[1993/06/16  20:16:30  motifdec]
 * 
 * $EndLog$
 */
/*
 */

#ifndef _XmCSTextSelP_h
#define _XmCSTextSelP_h

#include <Xm/Transfer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Boolean		has_destination;
    XmTextPosition 	position;
    long 		replace_length;
    Boolean 		quick_key;
    XmCSTextWidget 	widget;
} CSTextDestDataRec, *CSTextDestData;


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCSTextSelP_h */

/* DON'T ADD ANYTHING AFTER THIS #endif */
