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
/* $XConsortium: ScreenP.h /main/8 1995/07/13 17:53:51 drk $ */
/* (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/* (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/* (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/* (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef _XmScreenP_h
#define _XmScreenP_h

#include <Xm/DesktopP.h>
#include <Xm/Screen.h>
#include <Xm/DragIcon.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XmScreenClassPart {
    XtPointer		extension;
} XmScreenClassPart, *XmScreenClassPartPtr;

typedef struct _XmScreenClassRec {
/*    ObjectClassPart		object_class;
    XmExtClassPart		ext_class; */
    CoreClassPart               core_class ;
    XmDesktopClassPart 		desktop_class;
    XmScreenClassPart		screen_class;
} XmScreenClassRec;

typedef struct _XmDragCursorRec {
    struct _XmDragCursorRec	*next;
    Cursor			cursor;
    XmDragIconObject		stateIcon;
    XmDragIconObject		opIcon;
    XmDragIconObject		sourceIcon;
    Boolean			dirty;
} XmDragCursorRec, *XmDragCursorCache;

typedef struct _XmScratchPixmapKeyRec *XmScratchPixmapKey;

typedef struct _XmScratchPixmapKeyRec {
    Cardinal		depth;
    Dimension           width;
    Dimension           height;
} XmScratchPixmapKeyRec;

typedef struct {
    Boolean		mwmPresent;
    unsigned short	numReparented;
    int			darkThreshold;
    int			foregroundThreshold;
    int			lightThreshold;
    XmDragIconObject	defaultNoneCursorIcon;
    XmDragIconObject	defaultValidCursorIcon;
    XmDragIconObject	defaultInvalidCursorIcon;
    XmDragIconObject	defaultMoveCursorIcon;
    XmDragIconObject	defaultCopyCursorIcon;
    XmDragIconObject	defaultLinkCursorIcon;
    XmDragIconObject	defaultSourceCursorIcon;

    Cursor		nullCursor;
    XmDragCursorRec	*cursorCache;
    Cardinal		maxCursorWidth;
    Cardinal		maxCursorHeight;

    Cursor		menuCursor;
    unsigned char	unpostBehavior;
    XFontStruct *	font_struct;
    int			h_unit;
    int			v_unit;
    XtPointer		scratchPixmaps;
    unsigned char       moveOpaque;
    XmScreenColorProc   color_calc_proc;
    XmAllocColorProc    color_alloc_proc;
    XtEnum              bitmap_conversion_model;

    /* to save internally-created XmDragIcons */

    XmDragIconObject	xmStateCursorIcon;
    XmDragIconObject	xmMoveCursorIcon;
    XmDragIconObject	xmCopyCursorIcon;
    XmDragIconObject	xmLinkCursorIcon;
    XmDragIconObject	xmSourceCursorIcon;

    GC			imageGC;		/* OBSOLETE FIELD */
    int			imageGCDepth;           /* OBSOLETE FIELD */
    Pixel		imageForeground;        /* OBSOLETE FIELD */
    Pixel		imageBackground;        /* OBSOLETE FIELD */

    XtPointer		screenInfo;		/* extension */

    XtPointer           user_data;

    Pixmap              insensitive_stipple_bitmap;

#ifdef DEFAULT_GLYPH_PIXMAP
   Pixmap           default_glyph_pixmap ;
   unsigned int     default_glyph_pixmap_width ;
   unsigned int     default_glyph_pixmap_height ;
#endif

   XtPointer		inUsePixmaps;
} XmScreenPart, *XmScreenPartPtr;

typedef struct _XmScreenInfo {
	/* so much for information hiding */
	XtPointer	menu_state;		/* MenuUtil.c */
	Boolean		destroyCallbackAdded;	/* ImageCache.c */
} XmScreenInfo;

externalref XmScreenClassRec 	xmScreenClassRec;

typedef struct _XmScreenRec {
/*    ObjectPart			object;
    XmExtPart			ext; */
    CorePart                    core ;
    XmDesktopPart		desktop;
    XmScreenPart		screen;
} XmScreenRec;


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmScreenP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
