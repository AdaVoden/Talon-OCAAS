/*
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
 */
/*
 * HISTORY
 * $Log: CSTextOutP.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:50  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.1.9.7  1994/08/04  18:55:53  yak
 * 	Fixed problems with copyrights
 * 	[1994/08/04  18:39:51  yak]
 *
 * Revision 1.1.9.6  1994/06/06  21:12:47  dick
 * 	Improve the GetFontInfo caching scheme
 * 	[1994/06/06  19:53:17  dick]
 * 
 * Revision 1.1.9.5  1994/06/03  15:57:48  mryan
 * 	CR 9352 - cache GetFontInfo stuff in the widget output record.
 * 	[1994/06/03  15:53:32  mryan]
 * 
 * Revision 1.1.9.4  1994/06/01  21:11:42  drk
 * 	CR 6873: Resolve P.h name conflicts.
 * 	[1994/06/01  16:57:51  drk]
 * 
 * Revision 1.1.9.3  1994/05/25  15:26:43  dick
 * 	Make end of output line scanning work like XmText - CR 9278
 * 	[1994/05/25  15:24:55  dick]
 * 
 * Revision 1.1.9.2  1994/05/18  23:29:17  inca
 * 	Merged with changes from 1.1.9.1
 * 	[1994/05/18  23:28:37  inca]
 * 
 * 	Implement grow-only behavior for resizable text widgets
 * 	[1994/05/18  23:08:35  inca]
 * 
 * Revision 1.1.9.1  1994/05/18  13:20:43  mryan
 * 	CR 9220 - don't PaintCursor until the first expose, to avoid garbage
 * 	pixels around the cursor.
 * 	[1994/05/18  13:19:48  mryan]
 * 
 * Revision 1.1.6.14  1994/04/26  21:49:31  dick
 * 	Change over to using _XmStringEntry as the segment data type
 * 	[1994/04/26  21:26:06  dick]
 * 
 * Revision 1.1.6.13  1994/04/14  15:54:45  mryan
 * 	Add CursorOutLine field.
 * 	[1994/04/14  15:54:04  mryan]
 * 
 * Revision 1.1.6.12  1994/04/12  22:06:23  meeks
 * 	CR 8348: Added XmNfontList resource.
 * 	[1994/04/12  20:33:22  meeks]
 * 
 * Revision 1.1.6.11  1994/04/11  19:47:39  mryan
 * 	Performance - avoid always starting _draw() from top
 * 	[1994/04/11  19:46:46  mryan]
 * 
 * Revision 1.1.6.10  1994/03/29  20:50:42  mryan
 * 	Fix cursor ghosts problem
 * 	[1994/03/29  20:48:38  mryan]
 * 
 * Revision 1.1.6.9  1994/03/25  01:34:18  dick
 * 	Add the support for *-tab-beginning actions
 * 	[1994/03/25  01:31:04  dick]
 * 
 * Revision 1.1.6.8  1994/03/24  15:38:52  mryan
 * 	Fix build messages (Cursor macro).
 * 	[1994/03/24  15:12:42  mryan]
 * 
 * Revision 1.1.6.7  1994/03/23  19:42:06  mryan
 * 	Implement centering of overstrike cursors.
 * 	[1994/03/23  19:41:35  mryan]
 * 
 * Revision 1.1.6.6  1994/03/23  16:14:23  mryan
 * 	Redesign of CSText cursor handling.
 * 	[1994/03/23  16:09:37  mryan]
 * 
 * Revision 1.1.6.5  1994/03/18  21:07:11  mryan
 * 	Merged with changes from 1.1.6.4
 * 	[1994/03/18  21:07:06  mryan]
 * 
 * 	Support added for using ScrollFrame trait like XmText
 * 	[1994/03/18  20:59:02  mryan]
 * 
 * Revision 1.1.6.4  1994/03/18  20:35:44  dick
 * 	Fix r-to-l behavior that is incorrectly labelled BIDI
 * 	[1994/03/18  20:35:25  dick]
 * 
 * Revision 1.1.6.3  1994/03/10  22:29:20  inca
 * 	Latest drop from DEC
 * 	[1994/03/10  13:46:27  inca]
 * 
 * Revision 1.1.6.2  1994/03/04  22:21:22  drk
 * 	Hide _Xm routines from public view.
 * 	[1994/03/04  21:54:58  drk]
 * 
 * Revision 1.1.6.1  1994/02/15  18:13:27  inca
 * 	DEC bugfix for 7890
 * 	[1994/02/15  18:12:23  inca]
 * 
 * Revision 1.1.4.6  1993/12/06  16:33:47  inca
 * 	Patch from DEC for 7154, 7193 & 6729
 * 	[1993/12/06  16:24:49  inca]
 * 
 * Revision 1.1.4.5  1993/11/10  22:04:54  dick
 * 	Add XmNselectColor and the SetValues and Initialize code to handle the defaulting
 * 	[1993/11/09  18:50:15  dick]
 * 
 * Revision 1.1.4.4  1993/10/25  20:41:31  motifdec
 * 	Get rid of stuff no longer needed
 * 	[1993/10/25  14:30:19  motifdec]
 * 
 * Revision 1.1.4.3  1993/10/12  21:17:33  motifdec
 * 	Fixed some CRs relating to doc issues.
 * 	[1993/10/12  21:04:10  motifdec]
 * 
 * Revision 1.1.4.2  1993/09/16  02:28:54  motifdec
 * 	Some performance and restructuring with new XmString structs
 * 	[1993/09/16  02:15:26  motifdec]
 * 
 * Revision 1.1.2.8  1993/07/23  02:46:55  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:11:54  yak]
 * 
 * Revision 1.1.2.7  1993/07/19  20:22:02  motifdec
 * 	More Alpha fixes
 * 	[1993/07/19  19:48:29  motifdec]
 * 
 * Revision 1.1.2.6  1993/07/09  15:24:23  motifdec
 * 	More fixes to pass Alpha Acceptance tests
 * 	[1993/07/09  15:21:52  motifdec]
 * 
 * Revision 1.1.2.5  1993/06/16  20:18:51  motifdec
 * 	June 16 code drop from digital
 * 	[1993/06/16  19:00:59  motifdec]
 * 
 * $EndLog$
 */
/* @(#)$RCSfile: CSTextOutP.h,v $ $Revision: 1.1.1.1 $ (DEC) $Date: 2003/04/15 20:47:50 $ */

#ifndef _XmCSTextOutP_h
#define _XmCSTextOutP_h

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *
 * Definitions for modules implementing and using text output routines.
 *
 ****************************************************************/


/*
 * useful typedefs for the procs that an output module must supply
 */
#ifdef _NO_PROTO
typedef Boolean         (*CSBooleanProc)();
typedef XmTextPosition	(*CSXYToPosProc)();
typedef XmTextStatus	(*CSTextStatusProc)();

#else
typedef Boolean		(*CSBooleanProc)(
				XmCSTextWidget widget,
    				XmTextPosition position,
    				Position *x,
				Position *y);
typedef XmTextPosition (*CSXYToPosProc)(
				XmCSTextWidget widget,
#if NeedWidePrototypes
                        	int x,
                        	int y
#else
                        	Position x,
                        	Position y
#endif
				);
typedef XmTextStatus	(*CSTextStatusProc)(
    				XmCSTextWidget widget,
    				XmTextPosition start,
    				XmTextPosition end,
    				XmString *string);

#endif /* _NO_PROTO */

/* Rendition Manager */

typedef struct _RendMgrRec *RendMgr;

typedef struct _XmCSTextOutputDataRec 
{
    XmCSTextWidget 	widget;		/* Back-pointer to the widget */

    Widget 		vertical_scrollbar, 
			horizontal_scrollbar;

    Pixel 		foreground;	/* Color to use for text. */
    Pixel		select_color;	/* Color for highlighted region */
    XmFontList          font_list;	/* Field used by code. */
    XmRenderTable	render_table;   /* Field used to set font_list. */
    
    GC 			gc,
			image_gc,
                        insensitive_gc;
    GC                  save_gc;      /* GC for saving/restoring under IBeam */
    GC                  cursor_gc;   /* 1-bit depth GC for creating the I-beam 
					stipples (normal & add mode) */

    RendMgr		rendition_mgr;	/* Rendition Manager */

    Pixmap cursor;		/* Pixmap for IBeam cursor stencil. */
    Pixmap ltor_cursor;		/* Pixmap for LtoR IBeam cursor stencil. */
    Pixmap rtol_cursor;		/* Pixmap for RtoL IBeam cursor stencil. */
    Pixmap add_mode_cursor;	/* Pixmap to use for add mode cursor. */
    Pixmap ltor_add_mode_cursor;/* Pixmap to use for LtoR add mode cursor. */
    Pixmap rtol_add_mode_cursor;/* Pixmap to use for RtoL add mode cursor. */
    Pixmap ibeam_off;		/* Pixmap for area under the IBeam. */
    Pixmap stipple_tile;	/* stipple for add mode cursor. */
    unsigned int blinkrate;
    XtIntervalId timerid;
    _XmStringEntry  cursor_out_seg; /* Output segment & line containing the cursor */
    XmCSTextOutputLine  cursor_out_line;
    Pixmap 		error_char;	/* Pixmap for painting error chars */

    int                 vertical_offset,     /* how to get line x,y changed */
                        horizontal_offset;   /* to window x,y */

    int                 prev_top_edge;  /* to detect margin changes */
    int                 prev_left_edge; /* during setvalues */

    int                 line_count;     /* number of output lines */

    int 		leftmargin;	/* margin fields used by */
    int			rightmargin;	/* RowColumn constraints */
    int 		topmargin;
    int			bottommargin;

    int scrollwidth;		/* Total width of text we have to display. */
    int number_lines;		/* Number of lines that fit in the window. */

    /* Cached info used by GetFontInfo */
    char*               cached_charset;
    XmFontListEntry     cached_entry;
    XmTextType          cached_text_type;
    XmRenderTable	cached_fontlist;

    Dimension           max_descent,    /* font data */
                        max_ascent,
     		        min_width,      /* minimum sizes for resizable text */
			min_height;

    Dimension           typical_char_width,  /* found after perusing the */
                        typical_char_height; /* font list */

    short		rows,		/* number of rows */
    			columns,	/* number of columns */
    			rows_set,	/* history of previously */
			columns_set;	/* set dimensions */

    short int cursor_on;		/* Whether IBeam cursor is visible. */
    Position insertx, inserty;
    Dimension cursorwidth, cursorheight;
    Dimension curr_char_width;		/* Width of the character under the
					   overstrike cursor */

    Boolean 		wordwrap,	/* Whether to wordwrap. */
     		        hasfocus,       /* do we have focus now */
     	                doing_expose,
    		        ip_visible,     /* is insertion point visible */
     		        half_border,
                        bidirectional,  /* normal or bi-dir insert mode */
      		        resize_width,   /* resize policy */
			resize_height;

    Boolean 		scroll_vertical,     /* doing vertical scrolling ? */
			scroll_horizontal,   /* doing horizontal scrolling ? */
                        scroll_leftside,     /* is vert on the left */
			scroll_topside;      /* is horiz on top */

    Boolean 		ignore_vbar;	/* do we ignore callbacks from vbar.*/
    Boolean 		ignore_hbar;	/* do we ignore callbacks from hbar.*/

    Boolean		redisplay_all; 	/* specify all text need redisplay. */
    Boolean		need_vbar_update; /* specify vbar need recompute. */
    Boolean		need_hbar_update; /* specify hbar need recompute. */
    Boolean exposevscroll;	/* Non-zero if we expect expose events to be
				   off vertically. */
    Boolean exposehscroll;	/* Non-zero if we expect expose events to be
				   off horizontally. */
    Boolean suspend_hoffset;	/* temporarily suspend horizontal scrolling */
    Boolean refresh_ibeam_off;	/* Indicates whether area under IBeam needs
				 * to be re-captured */
    Boolean have_inverted_image_gc; /* fg/bg of image gc have been swapped;
				     * on == True, off == False */
    Boolean blinkstate;
    Boolean need_recompute;
    Boolean been_exposed;	/* We've been exposed once, so we can do
				 * display stuff now */
} XmCSTextOutputDataRec, *XmCSTextOutputData;

#define CST_OutputData(w)  ((XmCSTextOutputData)   \
                           ( ((XmCSTextOutput)(Output(w)) )->data))
 
#define CST_IsTwoByteFont(f)   (((f)->min_byte1 != 0 || (f)->max_byte1 != 0))

#define CST_Rows(w)		(CST_OutputData(w)->rows)
#define CST_Cols(w)		(CST_OutputData(w)->columns)

#define CST_InsertX(w)		(CST_OutputData (w)->insertx)
#define CST_InsertY(w)		(CST_OutputData (w)->inserty)
#define CST_CurrCharWidth(w)	(CST_OutputData (w)->curr_char_width)
#define CST_CursorOutSeg(w)	(CST_OutputData (w)->cursor_out_seg)
#define CST_CursorOutLine(w)	(CST_OutputData (w)->cursor_out_line)

#define CST_NormalGC(w)		(CST_OutputData (w)->gc)
#define CST_ImageGC(w)		(CST_OutputData (w)->image_gc)
#define CST_InsensitiveGC(w)	(CST_OutputData (w)->insensitive_gc)
#define CST_SaveGC(w)		(CST_OutputData (w)->save_gc)
#define CST_CursorGC(w)		(CST_OutputData (w)->cursor_gc)

#define CST_HalfBorder(w)	(CST_OutputData (w)->half_border)

#define CST_TypicalWidth(w)	(CST_OutputData (w)->typical_char_width)
#define CST_TypicalHeight(w)	(CST_OutputData (w)->typical_char_height)

#define CST_MaxDescent(w)	(CST_OutputData (w)->max_descent)
#define CST_MaxAscent(w)	(CST_OutputData (w)->max_ascent)
#define CST_MinWidth(w)         (CST_OutputData (w)->min_width)
#define CST_MinHeight(w)        (CST_OutputData (w)->min_height)

#define CST_DoingExpose(w)	(CST_OutputData (w)->doing_expose)

#define CST_IsLtoRTextPath(w)	(CSTextPath(w) == XmSTRING_DIRECTION_L_TO_R)
#define CST_IsRtoLTextPath(w)	(CSTextPath(w) == XmSTRING_DIRECTION_R_TO_L)

#define CST_CursorId(w)		(CST_OutputData (w)->cursor)
#define CST_CursorEnabled(w)	(CST_OutputData (w)->ip_visible)
#define CST_FontList(w)		(CST_OutputData (w)->font_list)
#define CST_LtoRCursor(w)	(CST_OutputData (w)->ltor_cursor)
#define CST_RtoLCursor(w)	(CST_OutputData (w)->rtol_cursor)
#define CST_AddModeCursor(w)	(CST_OutputData (w)->add_mode_cursor)
#define CST_LtoRAddModeCursor(w) (CST_OutputData (w)->ltor_add_mode_cursor)
#define CST_RtoLAddModeCursor(w) (CST_OutputData (w)->rtol_add_mode_cursor)
#define CST_IBeamOff(w)		(CST_OutputData (w)->ibeam_off)
#define CST_StippleTile(w)	(CST_OutputData (w)->stipple_tile)
#define CST_BlinkRate(w)	(CST_OutputData (w)->blinkrate)
#define CST_TimerId(w)		(CST_OutputData (w)->timerid)
#define CST_CursorOn(w)		(CST_OutputData (w)->cursor_on)
#define CST_CursorWidth(w)	(CST_OutputData (w)->cursorwidth)
#define CST_CursorHeight(w)	(CST_OutputData (w)->cursorheight)
#define CST_RefreshIBeamOff(w)	(CST_OutputData (w)->refresh_ibeam_off)
#define CST_HaveInvertedImageGC(w) (CST_OutputData (w)->have_inverted_image_gc)
#define CST_BlinkState(w)	(CST_OutputData (w)->blinkstate)

#define CST_PrevTopEdge(w)	(CST_OutputData (w)->prev_top_edge)
#define CST_PrevLeftEdge(w)	(CST_OutputData (w)->prev_left_edge)
#define CST_InnerWidget(w)	(((XmCSTextWidget)(w))->cstext.inner_widget)
#define CST_InnerWindow(w)	(XtWindow (CST_InnerWidget (w)))
#define CST_InnerWidth(w)	(XtWidth (CST_InnerWidget (w)))
#define CST_InnerHeight(w)	(XtHeight (CST_InnerWidget (w)))
#define CST_InnerX(w)		(XtX (CST_InnerWidget (w)))
#define CST_InnerY(w)		(XtY (CST_InnerWidget (w)))

#define CST_IsLToRSegment(s)	(SourceSegDirection(s) == XmSTRING_DIRECTION_L_TO_R)

#define CST_CachedCharset(w)    (CST_OutputData (w)->cached_charset)
#define CST_CachedEntry(w)      (CST_OutputData (w)->cached_entry)
#define CST_CachedTextType(w)   (CST_OutputData (w)->cached_text_type)
#define CST_CachedFontList(w)	(CST_OutputData (w)->cached_fontlist)

#define CST_VBar(w)		(CST_OutputData (w)->vertical_scrollbar)
#define CST_HBar(w)		(CST_OutputData (w)->horizontal_scrollbar)
#define CST_VerticalOffset(w)	(CST_OutputData (w)->vertical_offset)
#define CST_HorizontalOffset(w)	(CST_OutputData (w)->horizontal_offset)
#define CST_CanResizeWidth(w)	(CST_OutputData (w)->resize_width)
#define CST_CanResizeHeight(w)	(CST_OutputData (w)->resize_height)
#define CST_IgnoreVBar(w)	(CST_OutputData (w)->ignore_vbar)
#define CST_IgnoreHBar(w)	(CST_OutputData (w)->ignore_hbar)
#define CST_DoVScroll(w)	(CST_OutputData (w)->scroll_vertical)
#define CST_DoHScroll(w)	(CST_OutputData (w)->scroll_horizontal)
#define CST_VScrollOnLeft(w)	(CST_OutputData (w)->scroll_leftside)
#define CST_HScrollOnTop(w)	(CST_OutputData (w)->scroll_topside)

#define CST_LineCount(w)	(CST_OutputData (w)->line_count)
#define CST_TopMargin(w)	(CST_OutputData (w)->topmargin)
#define CST_BottomMargin(w)	(CST_OutputData (w)->bottommargin)
#define CST_LeftMargin(w)	(CST_OutputData (w)->leftmargin)
#define CST_RightMargin(w)	(CST_OutputData (w)->rightmargin)
  
#define CST_RedisplayAll(w)	(CST_OutputData(w)->redisplay_all)
#define CST_NeedVBarUpdate(w)	(CST_OutputData(w)->need_vbar_update)
#define CST_NeedHBarUpdate(w)	(CST_OutputData(w)->need_hbar_update)
#define CST_RenditionMgr(w)	(CST_OutputData(w)->rendition_mgr)

#define CST_WordWrap(w)		(CST_OutputData(w)->wordwrap)
#define CST_SelectColor(w)	(CST_OutputData(w)->select_color)

#define CST_ExposeVScroll(w)    (CST_OutputData(w)->exposevscroll)
#define CST_ExposeHScroll(w)    (CST_OutputData(w)->exposehscroll)
#define CST_SuspendHOffset(w)	(CST_OutputData(w)->suspend_hoffset)
#define CST_ScrollWidth(w)	(CST_OutputData(w)->scrollwidth)
#define CST_NumberLines(w)	(CST_OutputData(w)->number_lines)

#define CST_WidgetNeedRecompute(w) (CST_OutputData(w)->need_recompute)

#define CST_BeenExposed(w)	(CST_OutputData(w)->been_exposed)

/* Return the output line count for a specific widget.  The index is	*/
/* the value returned by the _find_widget function.			*/
#define CST_WidgetOutputLineCount(line, index) \
  (line->output_data[index].output_count)

/* Return a pointer to the first output line for a specific widget.	*/
/* The index is	the value returned by the _find_widget function.	*/
#define CST_FirstOutputLine(line, index) \
  (*(line->output_data[index].output_lines))

/* Return a pointer to a specific output line for a specific widget.	*/
/* The index is	the value returned by the _find_widget function.	*/
#define CST_NthOutputLine(line, index, n) \
  (line->output_data[index].output_lines[n])

/* Return the output segment count for a specific widget.  The index is	*/
/* the value returned by the _find_widget function.			*/
#define CST_WidgetOutputSegmentCount(segment, index) \
  (CST_SourceSegWidgetOutSegArray(segment)[index].output_count)

/* Return a pointer to the first output segment for a specific widget.	*/
/* The index is	the value returned by the _find_widget function.	*/
#define CST_FirstOutputSegment(segment, index) \
  (CST_SourceSegWidgetOutSegArray(segment)[index].output_segments[0])

/* Return a pointer to a specific output segment for a specific widget.	*/
/* The index is	the value returned by the _find_widget function.	*/
#define CST_NthOutputSegment(segment, index, n) \
  (CST_SourceSegWidgetOutSegArray(segment)[index].output_segments[n])

/*
 * various op codes that the output module HandleData method must
 * deal with
 */
typedef enum {
    XmCSTextOutputDelete = 0,		/* dump output data for everyth'g */

    XmCSTextOutputSegmentDelete,	/* dump output data for this seg */
    XmCSTextOutputSegmentAdd,		/* new seg added to source */
    XmCSTextOutputSegmentModified,	/* source changed this seg */

    XmCSTextOutputLineDelete,		/* dump output data for this line */
					/* this implies all segs of line */
    XmCSTextOutputLineAdd,		/* new line added to source */

    XmCSTextSegmentValidateData,	/* make sure output data for seg */
					/* is current */
    XmCSTextLineValidateData,		/* make sure output data for line */
					/* is current, implies all segs */
    XmCSTextValidateData,		/* make  output data for whole */
					/* source current */

    XmCSTextOutputLineModified,	/* source line changed something */

    XmCSTextOutputSingleLineDelete,	/* only that single line is deleted. */
					/* don't do any validation now */

    XmCSTextOutputSingleSegmentDelete,	/* only that single segment is */
					/* deleted.  don't do any */
					/* validation now */

    XmCSTextOutputRestLineValidate,	/* now, do validateion starting */
					/* from this line */

    XmCSTextOutputSingleLineInvalidate, /* invalidate this line */

    XmCSTextOutputTextEmpty 		/* Text becomes empty */
} XmCSTextDataOpCode;


#ifdef _NO_PROTO
typedef void (*CSDrawProc)();	
typedef void (*CSSetInsPointProc)();	
typedef void (*CSDrawInsPointProc)();	
typedef void (*CSSetCurProc)();	
typedef void (*CSMakePosVisProc)();	
typedef void (*CSHandleDataProc)();	
typedef void (*CSInvalidateProc)();	
typedef void (*CSGetValuesProc)();	
typedef Boolean (*CSSetValuesProc)();	
typedef void (*CSRealizeProc)();	
typedef void (*CSDestroyProc)();	
typedef void (*CSResizeProc)();	
typedef void (*CSRedisplayProc)();	
typedef void (*CSVertScrollProc)();	
typedef void (*CSHorScrollProc)();	
typedef void (*CSSetCharRedrawProc)();	
typedef void (*CSSetDrawModeProc)();	
typedef void (*CSComputeLineIndexProc)();	
typedef void (*CSSearchFontListProc)();	
typedef void (*CSNumLinesOnTextProc)();	
typedef void (*CSScanNextLineProc)();	
typedef void (*CSScanNextTabProc)();	
typedef void (*CSScanPrevLineProc)();	
typedef void (*CSScanPrevTabProc)();	
typedef void (*CSScanStartOfLineProc)();	
typedef void (*CSScanEndOfLineProc)();	
typedef void (*CSRedisplayHBarProc)();	
typedef void (*CSRedrawHalfBorderProc)();	
typedef void (*CSGetPreferredSizeProc)();
#else
typedef void (*CSDrawProc) 	(
				XmCSTextWidget widget
				);

typedef void (*CSSetInsPointProc) (
				XmCSTextWidget widget,
				XmTextPosition position
				);	

typedef void (*CSDrawInsPointProc) (
				XmCSTextWidget widget,
				XmTextPosition position,
#if NeedWidePrototypes
				int on
#else
				Boolean on
#endif
				);

typedef void (*CSMakePosVisProc) (
				XmCSTextWidget widget,
				XmTextPosition position
				);	

typedef void (*CSHandleDataProc) (
				XmCSTextWidget widget,
				XmCSTextLine line,
				_XmStringEntry segment,
			        XmTextPosition position,
				XmCSTextDataOpCode op_code
				);	

typedef void (*CSInvalidateProc) (
				XmCSTextWidget widget,
				XmTextPosition left,
				XmTextPosition right
				);	

typedef void (*CSGetValuesProc) (
				XmCSTextWidget widget,
				ArgList args,
				Cardinal num_args
				);	

typedef Boolean (*CSSetValuesProc) (
				XmCSTextWidget oldcsw,
				XmCSTextWidget reqcsw,
				XmCSTextWidget newcsw,
				ArgList args,
				Cardinal num_args
				);	

typedef void (*CSRealizeProc)	(
				XmCSTextWidget widget,
				Mask *valueMask,
				XSetWindowAttributes *attributes
				);	

typedef void (*CSDestroyProc) 	(
				XmCSTextWidget widget
				);	

typedef void (*CSResizeProc) 	(
				XmCSTextWidget widget
				);	

typedef void (*CSRedisplayProc) (
				XmCSTextWidget widget,
				XEvent *event
				);	

typedef void (*CSVertScrollProc) (
				XmCSTextWidget widget
				);	

typedef void (*CSHorScrollProc) (
				XmCSTextWidget widget
				);	

typedef void (*CSSetCharRedrawProc) (
				XmCSTextWidget widget,
				CSTextLocation location
				);	

typedef void (*CSSetDrawModeProc) (
				XmCSTextWidget widget,
				CSTextLocation location,
				unsigned int length,
				XmHighlightMode mode
				);	

typedef void (*CSComputeLineIndexProc) (
				XmCSTextWidget widget,
				CSTextLocation location,
				int *return_value
				);	

typedef void (*CSSearchFontListProc) (
				XmCSTextWidget widget,
				XmStringCharSet charset,
				XmFontListEntry *font,
				Boolean cache_tag
				);	

typedef void (*CSNumLinesOnTextProc) (
				XmCSTextWidget widget,
				int *num_lines
				);	

typedef void (*CSScanNextLineProc) (
				XmCSTextWidget widget,
				XmTextPosition position,
				CSTextLocation location,
				XmTextPosition *out_position
				);	

typedef void (*CSScanNextTabProc) (
				XmCSTextWidget widget,
				XmTextPosition position,
				CSTextLocation location,
				XmTextPosition *out_position
				);	

typedef void (*CSScanPrevLineProc) (
				XmCSTextWidget widget,
				XmTextPosition position,
				CSTextLocation location,
				XmTextPosition *out_position
				);	

typedef void (*CSScanPrevTabProc) (
				XmCSTextWidget widget,
				XmTextPosition position,
				CSTextLocation location,
				XmTextPosition *out_position
				);	

typedef void (*CSScanStartOfLineProc) (
				XmCSTextWidget widget,
				CSTextLocation location,
				XmTextPosition *out_position
				);	

typedef void (*CSScanEndOfLineProc) (
				XmCSTextWidget widget,
				CSTextLocation location,
				XmTextPosition *out_position,
#if NeedWidePrototypes
				int include
#else
				Boolean include
#endif
				);	

typedef void (*CSRedisplayHBarProc) (
				XmCSTextWidget widget
				);	

typedef void (*CSRedrawHalfBorderProc) (
				XmCSTextWidget widget
				);	
typedef void (*CSGetPreferredSizeProc)(
				Widget widget,
				Dimension *width,
				Dimension *height);
#endif

typedef struct _XmCSTextOutputRec 
{
    XmCSTextOutputData	data;

    /* known to be useful */
    CSDrawProc			Draw;
    CSSetInsPointProc		SetInsertionPoint;
    CSDrawInsPointProc          DrawInsertionPoint;
    CSMakePosVisProc		MakePositionVisible;
    CSHandleDataProc            HandleData;
    CSXYToPosProc		XYToPos;
    CSBooleanProc		PosToXY;
    CSInvalidateProc		Invalidate;
    CSGetValuesProc		GetValues;
    CSSetValuesProc		SetValues;
    CSRealizeProc		Realize;
    CSDestroyProc		Destroy;
    CSResizeProc		Resize;
    CSRedisplayProc	        Redisplay;
    CSVertScrollProc            VerticalScroll;
    CSHorScrollProc             HorizontalScroll;
    CSSetCharRedrawProc         SetCharRedraw;
    CSSetDrawModeProc           SetDrawMode;
    CSComputeLineIndexProc      ComputeLineIndex;
    CSSearchFontListProc        SearchFontList;
    CSNumLinesOnTextProc	NumLinesOnText;

/* Scanning routines dealing with logical lines
 */
    CSScanNextLineProc		ScanNextLine;
    CSScanNextTabProc		ScanNextTab;
    CSScanPrevLineProc		ScanPrevLine;
    CSScanPrevTabProc		ScanPrevTab;
    CSScanStartOfLineProc	ScanStartOfLine;
    CSScanEndOfLineProc		ScanEndOfLine;

/* Read String routine dealing with logical lines.  Note that '\n' is added
 * to the end of each out_line's string
 */
    CSTextStatusProc		ReadString;

    CSRedisplayHBarProc		RedisplayHBar;
    CSRedrawHalfBorderProc	RedrawHalfBorder;
    CSGetPreferredSizeProc	GetPreferredSize;
} XmCSTextOutputRec;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif
 
#endif /* _XmCSTextOutP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
