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
/* $XConsortium: CascadeBP.h /main/12 1996/03/25 14:52:42 pascale $ */
/* (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/* (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef  _XmCascadeBP_h
#define  _XmCascadeBP_h

#include <Xm/CascadeB.h>
#include <Xm/LabelP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The CascadeButton instance record */

typedef	struct 
{				/* resources */
    XtCallbackList	activate_callback;	/* widget fired callback */
    XtCallbackList	cascade_callback;	/* called when the menu is  */
						/* about to be pulled down */

    Widget		submenu;		/* the menu to pull down */
    Pixmap		cascade_pixmap;		/* pixmap for the cascade */
    int 		map_delay;		/* time delay for posting */

				/* internal fields */

    GC                  arm_GC;                 /* armed GC */
    GC                  background_GC;          /* normal GC */
    Boolean		armed;			/* armed flag */
    XRectangle		cascade_rect;		/* location of cascade*/
    XtIntervalId	timer;			/* timeout id */
    Pixmap		armed_pixmap;		/* arm arrow cascade */

} XmCascadeButtonPart;


/* Full instance record declaration */

typedef struct _XmCascadeButtonRec
{
    CorePart		core;
    XmPrimitivePart	primitive;
    XmLabelPart		label;
    XmCascadeButtonPart	cascade_button;
} XmCascadeButtonRec;

typedef struct _XmCascadeButtonWidgetRec/* OBSOLETE (for compatibility only).*/
{
    CorePart		core;
    XmPrimitivePart	primitive;
    XmLabelPart		label;
    XmCascadeButtonPart	cascade_button;
} XmCascadeButtonWidgetRec;


/* CascadeButton class structure */

typedef struct 
{
    XtPointer	extension;	/* Pointer to extension record */
} XmCascadeButtonClassPart;


/* Full class record declaration for CascadeButton class */

typedef struct _XmCascadeButtonClassRec 
{
    CoreClassPart	    core_class;
    XmPrimitiveClassPart    primitive_class;
    XmLabelClassPart	    label_class;
    XmCascadeButtonClassPart cascade_button_class;
} XmCascadeButtonClassRec;


externalref XmCascadeButtonClassRec   xmCascadeButtonClassRec;


/* Access macro definitions */

#define CB_Submenu(cb)		(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.submenu)

#define CB_ActivateCall(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.activate_callback)
#define CB_CascadeCall(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.cascade_callback)


#define CB_CascadePixmap(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.cascade_pixmap)
#define CB_ArmedPixmap(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.armed_pixmap)

#define CB_ArmGC(cb)            (((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.arm_GC)
#define CB_BackgroundGC(cb)     (((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.background_GC)

#define CB_Cascade_x(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.cascade_rect.x)
#define CB_Cascade_y(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.cascade_rect.y)
#define CB_Cascade_width(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.cascade_rect.width)
#define CB_Cascade_height(cb)	(((XmCascadeButtonWidget)                    \
                                  cb)->cascade_button.cascade_rect.height)

#define CB_HasCascade(cb)       (((Lab_MenuType(cb) == XmMENU_PULLDOWN)  ||  \
				  (Lab_MenuType(cb) == XmMENU_POPUP)) &&     \
			         (CB_Submenu(cb)))


#define XmCB_ARMED_BIT	      (1 << 0)	
#define XmCB_TRAVERSE_BIT     (1 << 1)
#define XmCB_WAS_POSTED_BIT   (1 << 2)


#define CB_IsArmed(cb)	 (((XmCascadeButtonWidget)(cb))->cascade_button.armed \
			  & XmCB_ARMED_BIT)

#define CB_Traversing(cb) (((XmCascadeButtonWidget)                           \
			    (cb))->cascade_button.armed & XmCB_TRAVERSE_BIT)

#define CB_WasPosted(cb)  (((XmCascadeButtonWidget)                           \
			    (cb))->cascade_button.armed & XmCB_WAS_POSTED_BIT)

#define CB_SetBit(byte,bit,v)  byte = (byte & (~bit)) | (v ? bit : 0)

#define CB_SetArmed(cb,v)  CB_SetBit (((XmCascadeButtonWidget)		     \
				       (cb))->cascade_button.armed,          \
				      XmCB_ARMED_BIT, v)

#define CB_SetTraverse(cb,v)  CB_SetBit (((XmCascadeButtonWidget)	     \
				       (cb))->cascade_button.armed,          \
				      XmCB_TRAVERSE_BIT, v)

#define CB_SetWasPosted(cb,v)  CB_SetBit (((XmCascadeButtonWidget)	     \
					   (cb))->cascade_button.armed,      \
					  XmCB_WAS_POSTED_BIT, v)



#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmCascadeBP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
