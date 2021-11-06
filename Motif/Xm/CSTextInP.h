/*
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
 */
/*
 * HISTORY
 * $Log: CSTextInP.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:50  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.1.8.2  1994/08/04  18:55:52  yak
 * 	Fixed problems with copyrights
 * 	[1994/08/04  18:39:48  yak]
 *
 * Revision 1.1.8.1  1994/06/01  21:11:28  drk
 * 	CR 6873: Resolve P.h name conflicts.
 * 	[1994/06/01  16:57:40  drk]
 * 
 * Revision 1.1.6.2  1994/03/28  23:22:31  inca
 * 	Rewrite Btn1Transfer code
 * 	[1994/03/28  22:57:13  inca]
 * 
 * Revision 1.1.6.1  1994/03/23  16:14:17  mryan
 * 	Redesign of CSText cursor handling.
 * 	[1994/03/23  16:08:45  mryan]
 * 
 * Revision 1.1.4.2  1993/09/16  02:28:33  motifdec
 * 	Some performance and restructuring with new XmString structs
 * 	[1993/09/16  02:15:16  motifdec]
 * 
 * Revision 1.1.2.8  1993/07/23  02:46:39  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:11:34  yak]
 * 
 * Revision 1.1.2.7  1993/07/19  20:21:49  motifdec
 * 	More Alpha fixes
 * 	[1993/07/19  19:48:12  motifdec]
 * 
 * Revision 1.1.2.6  1993/07/09  15:24:12  motifdec
 * 	More fixes to pass Alpha Acceptance tests
 * 	[1993/07/09  15:21:45  motifdec]
 * 
 * Revision 1.1.2.5  1993/06/16  20:18:42  motifdec
 * 	June 16 code drop from digital
 * 	[1993/06/16  18:59:59  motifdec]
 * 
 * $EndLog$
 */
/*
 */

/*
 * CSTextInP.h
 * for Motif release 2.0
 */

#ifndef _XmCSTextInP_h
#define _XmCSTextInP_h


#include <Xm/CSTextSrcP.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CST_InputData(w)  ((XmCSTextInputData)  \
             ( ((XmCSTextInput)(Input (w)) )->data))


/****************************************************************
 *
 * Definitions for modules implementing text input modules.
 *
 ****************************************************************/

typedef struct {
    int x;
    int y;
} _XmCSTextSelectionHint;

typedef struct _XmCSTextInputDataRec {
    XmCSTextWidget 		widget;	     	/* Back-ptr to widget record. */
    XmTextScanType 		*sarray;  	/* Description of what to cycle
                                                   through on selections. */
    int 			sarraycount;	/* # of elements in above */
    int				new_sel_length; /* New selection length for selection moves. */
    int 			threshold;	/* # of pixels crossed -> drag*/
    _XmCSTextSelectionHint 	selectionHint;	/* saved coords of button down*/
    _XmCSTextSelectionHint 	Sel2Hint;	/* saved coords of button down*/
    _XmTextActionRec          * transfer_action;/* to keep track of delayed 
						   action */
    XtIntervalId		select_id;
    XtIntervalId                drag_id;        /* timer to start btn1 drag */
    XmTextScanType 		stype;		/* Current selection type. */
    XmTextScanDirection 	extendDir;
    XmTextScanDirection 	Sel2ExtendDir;

    XmTextPosition 		origLeft,
				origRight;
    XmTextPosition 		Sel2OrigLeft,	
				Sel2OrigRight;
    XmTextPosition 		stuffpos;
    XmTextPosition		sel2Left,
				sel2Right;
    XmTextPosition 		anchor;		/* anchor point of the 
						   primary selection */
    Position 			select_pos_x;	/* x position for timer-based scrolling */
    Position 			select_pos_y;	/* y position for timer-based scrolling */
    Position 			verticalXPosition; 
						/* used for vertical moves */
    Position 			verticalYPosition; 
						/* used for vertical moves */

    Boolean 			pendingdelete;	/* TRUE if we're implementing 
						   pending delete */
    Boolean 			syncing;	/* If TRUE, then we've multiple
						   keystrokes */
    Boolean 			extending;	/* true if we are extending */
    Boolean 			Sel2Extending;	/* true if we are extending */
    Boolean 			hasSel2;	/* has secondary selection */
    Boolean 			has_destination;/* has destination selection */
    Boolean 			selectionMove;	/* delete selection after stuff */
    Boolean 			selectionLink;	/* ??? selection after stuff */
    Boolean 			cancel;		/* indicates that cancel was pressed */
    Boolean 			overstrike;	/* overstrike */
    Boolean 			sel_start;	/* indicates that a btn2 was pressed */
    Boolean 			pendingoff;	/* TRUE if we shouldn't do 
						   pending delete on the 
						   current selection. */
    Boolean 			verticalMoveLast;   
						/* true is last user cmnd was 
						   vertical move */

    Boolean 			quick_key;	/* 2ndary selection via the 
						   keyboard */
    Boolean 			changed_dest_visible;
						/* destination visibility changed */
    Time 			dest_time;	/* time of destination selection */
    Time 			sec_time;	/* time of secondary selection */
    Time 			lasttime;	/* Time of last event. */
    Time 			ppastetime;     /* Time paste event occurred 
						   (primary) */
    Time 			spastetime;	/* Time paste event occurred 
						   (secondary) */
    XComposeStatus 		compstatus;  	/* Compose key status. */  
} XmCSTextInputDataRec, *XmCSTextInputData;

#define CST_Overstrike(w) (CST_InputData(w)->overstrike)

#ifdef _NO_PROTO
typedef void (*XmCSTextInputInvalidateProc)(); /* widget, position, topos, delta*/
typedef void (*XmCSTextInputSetValuesProc)(); /* widget, args, num_args */
typedef void (*XmCSTextInputGetValuesProc)(); /* widget, args, num_args */
typedef void (*XmCSTextInputGetSecResProc)(); /* secResDataRtn */
#else
typedef void (*XmCSTextInputInvalidateProc) (
			XmCSTextWidget  widget,
			XmTextPosition	position,
			XmTextPosition	topos,
			int		delta);
typedef void (*XmCSTextInputSetValuesProc) (
			XmCSTextWidget	oldcsw,
			XmCSTextWidget 	reqcsw,
			XmCSTextWidget 	newcsw,
			ArgList 	 args,
			Cardinal 	 num_args );
typedef void (*XmCSTextInputGetValuesProc) (
			XmCSTextWidget	widget,
			ArgList        	args,
			Cardinal       	num_args);
typedef void (*XmCSTextInputGetSecResProc)(
			XmSecondaryResourceData *) ;
#endif /* _NO_PROTO */

typedef struct _XmCSTextInputRec {
    struct _XmCSTextInputDataRec 	*data; 
    XmCSTextInputInvalidateProc 	Invalidate;
    XmCSTextInputGetValuesProc  	GetValues;
    XmCSTextInputSetValuesProc		SetValues;
    XtWidgetProc			destroy;
    XmCSTextInputGetSecResProc          GetSecResData;
} XmCSTextInputRec;

typedef struct {
    Widget		widget;
    XmTextPosition 	insert_pos;
    int 		num_chars;
    Time 		timestamp;
    Boolean 		move;
} _XmCSTextDropTransferRec;


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCSTextInP_h */

/*DON'T ADD ANYTHING AFTER THIS #endif */
