/*
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
 */
/*
 * HISTORY
 * $Log: CSText.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:50  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.1.8.2  1994/08/04  18:55:45  yak
 * 	Fixed problems with copyrights
 * 	[1994/08/04  18:39:36  yak]
 *
 * Revision 1.1.8.1  1994/06/01  21:11:20  drk
 * 	CR 6873: Resolve P.h name conflicts.
 * 	[1994/06/01  16:57:30  drk]
 * 
 * Revision 1.1.6.2  1994/03/10  22:29:10  inca
 * 	Remove XmCSTextHorizontalScroll, CR 7328
 * 	[1994/03/10  22:02:41  inca]
 * 
 * Revision 1.1.6.1  1994/01/06  22:22:06  drk
 * 	Remove spurious XmCR_NOFONT definition.
 * 	[1994/01/06  22:21:54  drk]
 * 
 * Revision 1.1.4.5  1993/11/16  18:18:59  inca
 * 	Fix CR 6221 - Implemented XmCSTextSetTopCharacter for single-line
 * 	Also fixed a problem with the fix to 6220. This also took care of
 * 	the invisible cursor on empty lines
 * 	[1993/11/16  17:57:44  inca]
 * 
 * Revision 1.1.4.4  1993/11/11  22:28:17  inca
 * 	Change char * to String: CR 6808
 * 	[1993/11/11  17:54:59  inca]
 * 
 * Revision 1.1.4.3  1993/10/12  21:17:26  motifdec
 * 	Fixed some CRs relating to doc issues.
 * 	[1993/10/12  21:03:36  motifdec]
 * 
 * Revision 1.1.4.2  1993/09/16  02:28:20  motifdec
 * 	Some performance work. Some XmString restructuring.
 * 	[1993/09/16  02:13:47  motifdec]
 * 
 * Revision 1.1.2.12  1993/07/23  02:46:27  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:11:13  yak]
 * 
 * Revision 1.1.2.11  1993/07/22  18:23:29  drand
 * 	Added declarations for XmCSTextPasteLink and CopyLink.
 * 	[1993/07/22  14:47:14  drand]
 * 
 * Revision 1.1.2.10  1993/07/10  02:20:00  meeks
 * 	Moved XmTextStatus to Xm.h.
 * 	[1993/07/10  02:06:43  meeks]
 * 
 * Revision 1.1.2.9  1993/07/09  15:23:57  motifdec
 * 	More fixes to pass Alpha Acceptance tests
 * 	[1993/07/09  15:21:28  motifdec]
 * 
 * Revision 1.1.2.8  1993/07/02  17:20:14  motifdec
 * 	Added XmCSTextSetSource proto.
 * 	[1993/07/02  17:09:53  motifdec]
 * 
 * Revision 1.1.2.7  1993/06/30  15:28:32  rgcote
 * 	Moved the widget specific resource defines to XmStrDefs.
 * 	[1993/06/30  15:21:52  rgcote]
 * 
 * Revision 1.1.2.6  1993/06/16  20:18:32  motifdec
 * 	June 16 code drop from digital
 * 	[1993/06/16  18:57:40  motifdec]
 * 
 * $EndLog$
 */
/*
 */

/*
 * CSText.h
 * for Motif release 2.0
 */

#ifndef _XmCSText_h
#define _XmCSText_h

#include <Xm/Xm.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------- *
 *   type defines *
 * -------------- */
typedef struct _XmCSTextSourceRec *XmCSTextSource;
typedef struct _XmCSTextClassRec *XmCSTextWidgetClass;
typedef struct _XmCSTextRec *XmCSTextWidget;

/* -------------- *
 * extern class   *
 * -------------- */
externalref WidgetClass		xmCSTextWidgetClass;

/* --------------------------------------- *
 *  cstext widget fast subclassing fallback  *
 * --------------------------------------- */
#ifndef XmIsCSText
#define XmIsCSText(w)     XtIsSubclass(w, xmCSTextWidgetClass)
#endif /* XmIsCSText */
 

/* ----------------------------------- *
 *   cstext widget public functions    *
 * ----------------------------------- */

/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget		XmCreateCSText ();
extern Widget 		XmCreateScrolledCSText ();
extern XmString 	XmCSTextGetString ();
extern void 		XmCSTextSetString ();
extern void 		XmCSTextReplace ();
extern void 		XmCSTextRead ();
extern void 		XmCSTextInsert ();
extern Boolean 		XmCSTextGetEditable ();
extern void 		XmCSTextSetEditable ();
extern int 		XmCSTextGetMaxLength ();
extern void 		XmCSTextSetMaxLength ();
extern XmStringDirection XmCSTextGetTextPath ();
extern void 		XmCSTextSetTextPath ();
extern XmString 	XmCSTextGetSelection ();
extern Boolean 		XmCSTextGetSelectionPosition ();
extern void 		XmCSTextSetSelection ();
extern void 		XmCSTextClearSelection ();
extern Boolean 		XmCSTextCopy ();
extern Boolean 		XmCSTextCut ();
extern Boolean 		XmCSTextPaste ();
extern Boolean 		XmCSTextRemove ();
extern void 		XmCSTextShowPosition ();
extern void 		XmCSTextScroll ();
extern void 		XmCSTextDisableRedisplay ();
extern void 		XmCSTextEnableRedisplay ();
extern void 		XmCSTextMarkRedraw ();
extern void 		XmCSTextSetHighlight ();
extern XmTextPosition 	XmCSTextGetTopCharacter ();
extern void 		XmCSTextSetTopCharacter ();
extern XmTextPosition 	XmCSTextGetLastPosition ();
extern XmTextPosition 	XmCSTextGetCursorPosition ();
extern XmTextPosition 	XmCSTextGetInsertionPosition ();
extern void 		XmCSTextSetInsertionPosition ();
extern int 		XmCSTextNumLines ();
extern XmTextPosition 	XmCSTextXYToPos ();
extern Boolean 		XmCSTextPosToXY ();
extern XmString 	XmCSTextGetStringWrapped ();
extern Boolean 		XmCSTextFindString ();
extern Boolean 		XmCSTextFindStringWcs ();
extern int 		XmCSTextGetSubstring ();
extern int 		XmCSTextGetSubstringWcs ();
extern void		XmCSTextSetAddMode ();
extern void		XmCSTextSetSource ();
extern XmCSTextSource	XmCSTextGetSource ();
extern int		XmCSTextGetBaseline ();
extern Boolean		XmCSTextPasteLink ();
extern Boolean          XmCSTextCopyLink ();
#else /* _NO_PROTO */

extern Widget XmCreateCSText (
			Widget 		parent,
			String		name,
			ArgList 	arglist,
			Cardinal 	argcount);
extern Widget XmCreateScrolledCSText (
			Widget 		parent,
			String		name,
			ArgList 	arglist,
			Cardinal 	argcount);
extern XmString XmCSTextGetString (
			Widget 		widget);
extern void XmCSTextSetString (
			Widget 		widget,
			XmString 	value);
extern void XmCSTextReplace (
			Widget 		widget,
			XmTextPosition 	from_pos,
			XmTextPosition 	to_pos,
			XmString 	value);
extern void XmCSTextRead (
			Widget 		widget,
			XmTextPosition 	frompos,
			XmTextPosition 	topos,
			XmString 	*value);
extern void XmCSTextInsert (
			Widget 		widget,
			XmTextPosition 	position,
			XmString 	value);
extern Boolean XmCSTextGetEditable (
			Widget 		widget);
extern void XmCSTextSetEditable (
			Widget 		widget,
#if NeedWidePrototypes
			int 		editable);
#else
			Boolean 	editable);
#endif /* NeedWidePrototypes */
extern int XmCSTextGetMaxLength (
			Widget 		widget);
extern void XmCSTextSetMaxLength (
			Widget 		widget,
			int 		max_length);
extern XmStringDirection XmCSTextGetTextPath (
			Widget 		widget);
extern void XmCSTextSetTextPath (
			Widget 		widget,
			XmStringDirection path);
extern XmString XmCSTextGetSelection (
			Widget 		widget);
extern Boolean XmCSTextGetSelectionPosition (
			Widget 		widget,
			XmTextPosition 	*left,
			XmTextPosition 	*right);
extern void XmCSTextSetSelection (
			Widget 		widget,
			XmTextPosition 	first,
			XmTextPosition 	last,
			Time 		time);
extern void XmCSTextClearSelection (
			Widget 		widget,
			Time 		time);
extern Boolean XmCSTextCopy (
			Widget 		widget,
			Time 		time);
extern Boolean XmCSTextCut (
			Widget 		widget,
			Time 		time);
extern Boolean XmCSTextPaste (
			Widget 		widget);
extern Boolean XmCSTextRemove (
			Widget		widget);
extern void XmCSTextShowPosition (
			Widget 		widget,
			XmTextPosition 	position);
extern void XmCSTextScroll (
			Widget 		widget,
			int 		lines);
extern void XmCSTextDisableRedisplay (
			Widget 		widget);
extern void XmCSTextEnableRedisplay (
			Widget 		widget);
extern void XmCSTextMarkRedraw (
			Widget 		widget,
			XmTextPosition 	left,
			XmTextPosition 	right);
extern void XmCSTextSetHighlight (
			Widget 		widget,
			XmTextPosition 	left,
			XmTextPosition 	right,
			XmHighlightMode	mode);
extern XmTextPosition XmCSTextGetTopCharacter (
			Widget 		widget);
extern void XmCSTextSetTopCharacter (
			Widget 		widget,
			XmTextPosition 	top_position);
extern XmTextPosition XmCSTextGetLastPosition (
			Widget 		widget);
extern XmTextPosition XmCSTextGetCursorPosition (
			Widget 		widget);
extern XmTextPosition XmCSTextGetInsertionPosition (
			Widget 		widget);
extern void XmCSTextSetInsertionPosition (
			Widget 		widget,
			XmTextPosition	position);
extern XmTextPosition XmCSTextXYToPos(
			Widget 		widget,
#if NeedWidePrototypes
			int 		x,
			int 		y);
#else
			Position 	x,
			Position 	y);
#endif /* NeedWidePrototypes */
extern Boolean XmCSTextPosToXY (
			Widget 		widget,
			XmTextPosition 	position,
			Position 	*x,
			Position 	*y);
extern XmString XmCSTextGetStringWrapped (
			Widget 		widget,
			XmTextPosition	start,
			XmTextPosition	end);

extern Boolean XmCSTextFindString (
			Widget 		widget,
			XmTextPosition	start,
			String          string,
			XmTextDirection	direction,
			XmTextPosition	*position );

extern Boolean XmCSTextFindStringWcs (
        		Widget 		widget,
        		XmTextPosition	start,
        		wchar_t		*wcstring,
        		XmTextDirection	direction,
        		XmTextPosition	*position );

extern int XmCSTextGetSubstring (
			Widget   	widget,
			XmTextPosition 	start,
			int 		num_chars,
			int 		buffer_size,
			String 		buffer  );

extern int XmCSTextGetSubstringWcs (
			Widget   	widget,
			XmTextPosition 	start,
			int 		num_chars,
			int 		buffer_size,
			wchar_t		*buffer );

extern void XmCSTextSetAddMode (
			Widget		widget,
#if NeedWidePrototypes
			int		state );
#else
			Boolean		state );
#endif /* NeedWidePrototypes */

extern void XmCSTextSetSource (
			Widget   	widget,
        		XmCSTextSource 	source,
        		XmTextPosition 	top_character,
        		XmTextPosition 	cursor_position );
extern XmCSTextSource XmCSTextGetSource (
			Widget 		widget );
extern int XmCSTextGetBaseline (
			Widget		widget );
extern Boolean XmCSTextPasteLink (
			Widget		widget );
extern Boolean XmCSTextCopyLink (
                        Widget          widget,
                        Time            time);
#endif  /* _NO_PROTO */

typedef struct
{
    int                 reason;
    XEvent              *event;
    Boolean             doit;
    XmTextPosition   	currInsert,
                        newInsert;
    XmTextPosition   	startPos,
	                endPos;
    XmString            text;
} XmCSTextVerifyCallbackStruct, *XmCSTextVerifyPtr;


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif
 
#endif /*  _XmCSText_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
