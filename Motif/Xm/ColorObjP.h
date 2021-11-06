/* $XConsortium: ColorObjP.h /main/10 1996/12/16 18:30:49 drk $ */
/*
 * COPYRIGHT NOTICE
 * Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 * ALL RIGHTS RESERVED (MOTIF).  See the file named COPYRIGHT.MOTIF
 * for the full copyright text.
 * 
 */
/*
 * HISTORY
 */
#ifndef _XmColorObjP_h
#define _XmColorObjP_h

#include <Xm/VendorSP.h>

#ifdef __cplusplus
extern "C" {
#endif

/** misc structures, defines, and functions for using ColorObj **/

#define XmCO_MAX_NUM_COLORS	 8
#define XmCO_NUM_COLORS		 XmCO_MAX_NUM_COLORS
#define XmPIXEL_SET_PROP_VERSION '1'

/* Constants for color usage */
enum { XmCO_BLACK_WHITE, XmCO_LOW_COLOR, XmCO_MEDIUM_COLOR, XmCO_HIGH_COLOR };

typedef struct {
    Pixel fg;
    Pixel bg;
    Pixel ts;
    Pixel bs;
    Pixel sc;
} XmPixelSet;

typedef XmPixelSet Colors[XmCO_NUM_COLORS];

typedef struct _XmColorObjPart {
    XtArgsProc          RowColInitHook;
    XmPixelSet       	*myColors;     /* colors for my (application) screen */
    int             	myScreen;
    Display             *display;     /* display connection for "pseudo-app" */
    Colors         	*colors;      /* colors per screen for workspace mgr */
    int             	numScreens;   /*               for workspace manager */
    Atom           	*atoms;       /* to identify colorsrv screen numbers */
    Boolean         	colorIsRunning;   /* used for any color problem      */
    Boolean         	done;
    int            	*colorUse;
    int             	primary;
    int             	secondary;
    int             	text;          /* color set id for text widgets */
    int             	active;
    int             	inactive;
    Boolean         	useColorObj;  /* read only resource variable */
    Boolean             useText;        /* use text color set id for text? */
    Boolean             useTextForList; /* use text color set id for lists? */
    
    Boolean		useMask;
    Boolean		useMultiColorIcons;
    Boolean		useIconFileCache;

} XmColorObjPart;


typedef struct _XmColorObjRec {
    CorePart 		core;
    CompositePart 	composite;
    ShellPart 		shell;
    WMShellPart		wm;
    XmColorObjPart	color_obj;
} XmColorObjRec;

typedef struct _XmColorObjClassPart {
    XtPointer        extension;
} XmColorObjClassPart;

/* 
 * we make it a appShell subclass so it can have it's own instance
 * hierarchy
 */
typedef struct _XmColorObjClassRec{
    CoreClassPart      		core_class;
    CompositeClassPart 		composite_class;
    ShellClassPart  		shell_class;
    WMShellClassPart   		wm_shell_class;
    XmColorObjClassPart		color_obj_class;
} XmColorObjClassRec;


externalref XmColorObjClassRec xmColorObjClassRec;


#ifndef XmIsColorObj
#define XmIsColorObj(w) (XtIsSubclass(w, xmColorObjClass))
#endif /* XmIsXmDisplay */

externalref WidgetClass  xmColorObjClass;
typedef struct _XmColorObjClassRec *XmColorObjClass;
typedef struct _XmColorObjRec      *XmColorObj;


#define  XmCO_DitherTopShadow(display, screen, pixelSet) \
                        ((pixelSet)->bs == BlackPixel((display), (screen)))

#define  XmCO_DitherBottomShadow(display, screen, pixelSet) \
                        ((pixelSet)->ts == WhitePixel((display), (screen)))

#define  XmCO_DITHER     XmS50_foreground
#define  XmCO_NO_DITHER  XmSunspecified_pixmap


/********    Private Function Declarations    ********/

extern Boolean XmeGetIconControlInfo( 
                        Screen *screen,
                        Boolean *useMaskRtn,
                        Boolean *useMultiColorIconsRtn,
                        Boolean *useIconFileCacheRtn) ;

extern Boolean XmeUseColorObj( void ) ;


extern Boolean XmeGetColorObjData(
                   Screen * screen,
                   int *colorUse,
		   XmPixelSet *pixelSet,
		   unsigned short num_pixelSet,
		   short *active_id,
		   short *inactive_id,
		   short *primary_id,
		   short *secondary_id,
		   short *text_id) ;

extern Boolean XmeGetDesktopColorCells (
                         Screen * screen, 
			 Colormap colormap, 
			 XColor * colors,  
			 int n_colors,     
			 int * ncolors_returns) ;

/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmColorObjP_h */

