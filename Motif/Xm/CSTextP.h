/*
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
 */
/*
 * HISTORY
 * $Log: CSTextP.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:50  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.1.8.1  1994/08/04  18:55:54  yak
 * 	Fixed problems with copyrights
 * 	[1994/08/04  18:39:53  yak]
 *
 * Revision 1.1.6.2  1994/04/12  12:46:37  dick
 * 	Add default TextType to widget
 * 	[1994/04/12  12:45:21  dick]
 * 
 * Revision 1.1.6.1  1994/03/04  22:21:23  drk
 * 	Hide _Xm routines from public view.
 * 	[1994/03/04  21:54:59  drk]
 * 
 * Revision 1.1.4.4  1993/11/09  20:54:02  inca
 * 	Wrong class name for CSText
 * 	[1993/11/09  20:48:58  inca]
 * 
 * Revision 1.1.4.3  1993/10/27  20:05:36  dick
 * 	Modified enable/disable redisplay to consistently touch the  widgets sharing a source
 * 	[1993/10/27  19:56:17  dick]
 * 
 * Revision 1.1.4.2  1993/09/16  02:29:00  motifdec
 * 	Some performance and restructuring with new XmString structs
 * 	[1993/09/16  02:14:43  motifdec]
 * 
 * Revision 1.1.2.8  1993/07/23  02:46:57  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:11:59  yak]
 * 
 * Revision 1.1.2.7  1993/07/19  20:22:04  motifdec
 * 	More Alpha fixes
 * 	[1993/07/19  19:48:35  motifdec]
 * 
 * Revision 1.1.2.6  1993/07/02  17:20:30  motifdec
 * 	Removed _XmCSTextGetDropReceiver proto, renamed text_path to string_direction.
 * 	[1993/07/02  17:11:56  motifdec]
 * 
 * Revision 1.1.2.5  1993/06/16  20:18:53  motifdec
 * 	June 16 code drop from digital
 * 	[1993/06/16  20:16:04  motifdec]
 * 
 * $EndLog$
 */
/*
 */

/*
 * CSTextP.h
 * for Motif release 2.0
 */

#ifndef _XmCSTextP_h
#define _XmCSTextP_h

#include <Xm/PrimitiveP.h>
#include <Xm/Transfer.h>
#include <Xm/CSText.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSTEXTWIDGETCLASS "XmCSText"	/* Resource class name for CSText */

typedef struct {
  XtPointer         extension;      /* Pointer to extension record */
} CSTextClassPart;

typedef struct _XmCSTextClassRec {
  CoreClassPart 	core_class;
  XmPrimitiveClassPart 	primitive_class;
  CSTextClassPart 	cstext_class;
} XmCSTextClassRec, *XmCSTextClass;

externalref XmCSTextClassRec xmCSTextClassRec;


#ifdef _NO_PROTO

typedef void (*XmCSTextOutputCreateProc)();
typedef void (*XmCSTextInputCreateProc)();

#else

typedef void (*XmCSTextOutputCreateProc)(
			XmCSTextWidget	widget,
			ArgList		args,
			Cardinal	num_args);
typedef void (*XmCSTextInputCreateProc)(
			XmCSTextWidget	widget,
			ArgList		args,
			Cardinal	num_args);

#endif /* _NO_PROTO */

typedef struct _XmCSTextInputRec 	*XmCSTextInput;
typedef struct _XmCSTextOutputRec 	*XmCSTextOutput;
typedef struct _CSTextLine		*XmCSTextLine;
typedef struct _XmCSTextOutputLineRec	*XmCSTextOutputLine;

typedef struct _XmCSTextPart 
{
    XmCSTextOutput    	output;
    XmCSTextInput     	input;
    XmCSTextOutputLine  *output_lines;	     /* Output lines info	*/
    Cardinal		output_count;	     /* Number of output lines	*/
    int                 widget_index;        /* Index of this widget into
					      *	shared source struct 
					      */
    XmCSTextSource	source;		     /* Shared source info	*/
    XmCSTextOutputCreateProc  output_create; /* Creates output portion. */
    XmCSTextInputCreateProc   input_create;  /* Creates input portion.  */

    XtCallbackList    	activate_callback;
    XtCallbackList    	focus_callback;
    XtCallbackList    	losing_focus_callback;
    XtCallbackList    	value_changed_callback;
    XtCallbackList    	modify_verify_callback;
    XtCallbackList    	motion_verify_callback;
    XtCallbackList    	gain_primary_callback;
				/* Gained ownership of Primary Selection     */
    XtCallbackList    	lose_primary_callback;
				/* Lost ownership of Primary Selection       */

    XtCallbackList    	destination_callback;
    XmString 	      	value;
    XmCSTextLine       	lines;		/* source's data structures hook     */

    int 	      	max_length;	/* max allowable length of string    */

    XmDirection  	string_direction;
    XmStringDirection  	editing_path;
    int 	      	edit_mode;	/* Sets the line editing mode        */

    Dimension 	      	margin_height;
    Dimension         	margin_width;

    XmTextPosition	top_character;   /* First position to display.        */
    XmTextPosition	bottom_position; /* Last position to display.        */
    XmTextPosition	cursor_position; /* Location of the insertion point. */
    XmTextPosition	new_top;	/* Desired new top position.         */
    XmTextPosition	dest_position;	/* Location of the destination point */

    Boolean 	      	add_mode;	/* Determines the state of add mode  */
    Boolean 	      	auto_show_cursor_position; 
					/* do we automatically try to show   */
					/* the cursor position whenever it   */
					/* changes.                          */
    Boolean 	      	editable;	/* Determines if text is editable    */
    Boolean 	      	traversed;	/* Flag used with losing focus       */
					/* verification to indicate a        */
                                        /* traversal key pressed event       */
    Boolean 	      	needs_redisplay; /* If need to do things             */
    Boolean 	      	needs_refigure_lines;
    Boolean 	      	in_redisplay;   /* If in various proc's              */
    Boolean 	      	in_resize;
    Boolean 	      	in_refigure_lines;
    Boolean	      	has_focus;      /* Does the widget have the focus?   */
    Boolean	      	verify_bell;	/* Determines if bell is sounded     */
					/* when verify callback returns	     */
    					/* doit - False			     */

    int 	      	disable_depth;  /* How many levels of disable we've  */
					/* done.                             */

                                        /* current unhandled scroll demands  */
    int		      	pending_vertical_scroll;
    int                	pending_horiz_scroll;

    Widget 	      	inner_widget;	/* Ptr to widget which really has    */
				        /* text (may be same or different    */
                                        /* from this widget                  */

    XmStringCharSet	def_charset;     /* char set to be used in SelfInsert */
    XmTextType		def_text_type;	/* fallback textType */
} XmCSTextPart;


typedef struct _XmCSTextRec 
{
    CorePart		core;
    XmPrimitivePart 	primitive;
    XmCSTextPart 	cstext;
} XmCSTextRec;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCSTextP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
