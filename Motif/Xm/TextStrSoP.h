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
/* $XConsortium: TextStrSoP.h /main/9 1995/07/13 18:11:11 drk $ */
/* (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/* (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmTextStrSoP_h
#define _XmTextStrSoP_h

#include <Xm/XmP.h>
#include <Xm/Text.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************
 *
 * Definitions for use by sources and source users.
 *
 ****************************************************************/

typedef struct _XmSourceDataRec {
  XmTextSource source;	/* Backpointer to source record. */
  XmTextWidget *widgets;	/* Array of widgets displaying this source. */
  XmTextPosition left, right; /* Left and right extents of selection. */
  char * ptr;			/* Actual string data. */
  char * value;		/* Value of the string data. */
  char * gap_start;		/* Gapped buffer start pointer */
  char * gap_end;		/* Gapped buffer end pointer */
  char * PSWC_NWLN;           /* Holder for char*, short*, int* rep of NWLN */
  int length;			/* Number of chars of data. */
  int maxlength;		/* Space allocated. */
  int old_length;		/* Space allocated for value pointer. */
  int numwidgets;		/* Number of entries in above. */
  int maxallowed;		/* The user is not allowed to grow source */
  /* to a size greater than this. */
  Time prim_time;             /* time of primary selection */
  Boolean hasselection;	/* Whether we own the selection. */
  Boolean editable;		/* Whether we allow any edits. */
  Boolean take_selection;	/* Whether we should take the selection. */
} XmSourceDataRec, *XmSourceData;

typedef void (*AddWidgetProc)(XmTextSource,
			      XmTextWidget);

typedef int (*CountLinesProc)(XmTextSource,
			      XmTextPosition,
			      unsigned long);

typedef void (*RemoveWidgetProc)(XmTextSource,
				 XmTextWidget);

typedef XmTextPosition (*ReadProc)(XmTextSource,
				   XmTextPosition,	/* starting position */
				   XmTextPosition,	/* The last position 
							   we're interested in.
							   Don't return info 
							   about any later
							   positions. */
				   XmTextBlock);	/* RETURN: text read */

typedef XmTextStatus (*ReplaceProc)(XmTextWidget,
				    XEvent *,
				    XmTextPosition *,
				    XmTextPosition *,
				    XmTextBlock,
#if NeedWidePrototypes
				    int);
#else
                                    Boolean);
#endif /* NeedsWidePrototypes */

typedef XmTextPosition (*ScanProc)(XmTextSource,
				   XmTextPosition,
				   XmTextScanType,
				   XmTextScanDirection,	/*XmsdLeft/XmsdRight*/
				   int,
#if NeedWidePrototypes
				   int);
#else
    		                   Boolean);
#endif /* NeedsWidePrototypes */

typedef Boolean (*GetSelectionProc)(XmTextSource,
                                    XmTextPosition *,
                                    XmTextPosition *);

typedef void (*SetSelectionProc)(XmTextSource,
				 XmTextPosition,
				 XmTextPosition,
				 Time);


typedef struct _XmTextSourceRec {
  struct _XmSourceDataRec *data;   /* Source-defined data (opaque type). */
  AddWidgetProc	AddWidget;
  CountLinesProc	CountLines;
  RemoveWidgetProc	RemoveWidget;
  ReadProc		ReadSource;
  ReplaceProc		Replace;
  ScanProc		Scan;
  GetSelectionProc	GetSelection;
  SetSelectionProc	SetSelection;
} XmTextSourceRec;


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /*  _XmTextStrSoP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
