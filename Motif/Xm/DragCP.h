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
/*   $XConsortium: DragCP.h /main/12 1996/10/17 16:45:27 cde-osf $ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmDragCP_h
#define _XmDragCP_h

#include <Xm/XmP.h>
#include <Xm/DragC.h>

#include <X11/Shell.h>
#include <X11/ShellP.h>

#include <Xm/DragIcon.h>
#include <Xm/DragOverS.h>
#include <Xm/DropSMgrP.h>

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
 *
 * DragContext (RootWrapper) Widget Private Data
 *
 ***********************************************************************/

typedef void	(*XmDragStartProc)( XmDragContext, Widget, XEvent *);
typedef void 	(*XmDragCancelProc)( XmDragContext) ;


typedef struct {
    XmDragStartProc		start;
    XmDragCancelProc		cancel;
    XtPointer       		extension;
} XmDragContextClassPart;

typedef struct _XmDragContextClassRec {
    CoreClassPart	      	core_class;
    XmDragContextClassPart	drag_class;
} XmDragContextClassRec;

externalref XmDragContextClassRec xmDragContextClassRec;

#define XtDragByPoll 	0
#define XtDragByEvent	1

typedef struct {
    Window		frame;
    Window		window;
    Widget		shell;
    unsigned char	flags;
    unsigned char	dragProtocolStyle;
    int			xOrigin, yOrigin;
    unsigned int	width, height;
    unsigned int	depth;
    XtPointer		iccInfo;
} XmDragReceiverInfoStruct, *XmDragReceiverInfo;


typedef union _XmConvertSelectionRec
  {
    XtConvertSelectionIncrProc sel_incr ;
    XtConvertSelectionProc     sel ;
  } XmConvertSelectionRec ;
  

typedef struct _XmDragContextPart{
    /****  resources ****/

    Atom			*exportTargets;
    Cardinal			numExportTargets;
    XmConvertSelectionRec	convertProc;
    XtPointer			clientData;
    XmDragIconObject		sourceCursorIcon;
    XmDragIconObject		stateCursorIcon;
    XmDragIconObject		operationCursorIcon;
    XmDragIconObject		sourcePixmapIcon;
    Pixel			cursorBackground;
    Pixel			cursorForeground;
    Pixel			validCursorForeground;
    Pixel			invalidCursorForeground;
    Pixel			noneCursorForeground;
    XtCallbackList		dragMotionCallback;
    XtCallbackList		operationChangedCallback;
    XtCallbackList		siteEnterCallback;
    XtCallbackList		siteLeaveCallback;
    XtCallbackList		topLevelEnterCallback;
    XtCallbackList		topLevelLeaveCallback;
    XtCallbackList		dropStartCallback;
    XtCallbackList		dropFinishCallback;
    XtCallbackList		dragDropFinishCallback;
    unsigned char		dragOperations;
    Boolean			incremental;
    unsigned char		blendModel;

    /* private resources */
    Window			srcWindow;
    Time			dragStartTime;
    Atom			iccHandle;
    Widget			sourceWidget;
    Boolean			sourceIsExternal;

    /**** instance data ****/
    Boolean			topWindowsFetched;
    unsigned char 		commType;
    unsigned char		animationType;

    unsigned char		operation;
    unsigned char		operations;
    unsigned int		lastEventState;
    unsigned char		dragCompletionStatus;
    unsigned char		dragDropCompletionStatus;
    Boolean			forceIPC;
    Boolean			serverGrabbed;
    Boolean			useLocal;
    Boolean			inDropSite;
    XtIntervalId 		dragTimerId;
    
    Time			roundOffTime;
    Time			lastChangeTime;
    Time			crossingTime;

    Time			dragFinishTime;
    Time			dropFinishTime;
    
    Atom			dropSelection;
    Widget			srcShell;
	Position		startX, startY;

    XmID			siteID;

    Screen			*currScreen;
    Window			currWmRoot;
    XmDragOverShellWidget	curDragOver;
    XmDragOverShellWidget	origDragOver;

    XmDragReceiverInfoStruct	*currReceiverInfo;
    XmDragReceiverInfoStruct	*rootReceiverInfo;
    XmDragReceiverInfoStruct	*receiverInfos;
    Cardinal			numReceiverInfos;
    Cardinal			maxReceiverInfos;

    unsigned char		trackingMode;
    unsigned char		activeProtocolStyle;
    unsigned char               activeBlendModel;
    Boolean			dragDropCancelEffect;
    long 			SaveEventMask; 		/* Save the current root eventMask so that D&D works for MWM */
} XmDragContextPart;


typedef  struct _XmDragContextRec{
    CorePart	 		core;
    XmDragContextPart		drag;
} XmDragContextRec;


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDragCP_h */
