/*
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
 */
/*
 * HISTORY
 * $Log: CSTextSrcP.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:51  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.1.9.5  1994/08/04  18:56:00  yak
 * 	Fixed problems with copyrights
 * 	[1994/08/04  18:40:08  yak]
 *
 * Revision 1.1.9.4  1994/07/26  14:51:08  inca
 * 	Fix C++ compiler errors/warnings
 * 	[1994/07/26  14:16:36  inca]
 * 
 * Revision 1.1.9.3  1994/07/18  15:49:56  mryan
 * 	CR 9778 - replace C++-bogus cache creation macros.
 * 	[1994/07/18  15:48:16  mryan]
 * 
 * Revision 1.1.9.2  1994/06/01  21:11:51  drk
 * 	CR 6873: Resolve P.h name conflicts.
 * 	[1994/06/01  16:58:07  drk]
 * 
 * Revision 1.1.9.1  1994/05/11  11:56:50  dick
 * 	Make all character traversal work with widget fontlist
 * 	[1994/05/11  11:50:27  dick]
 * 
 * Revision 1.1.7.12  1994/05/06  15:07:35  dick
 * 	Fix ! problem
 * 	[1994/05/06  14:55:56  dick]
 * 
 * Revision 1.1.7.11  1994/04/28  21:39:07  dick
 * 	Change prev in the convert context to be just direction
 * 	[1994/04/28  14:50:38  dick]
 * 
 * Revision 1.1.7.10  1994/04/27  13:16:40  mryan
 * 	Merged with changes from 1.1.7.9
 * 	[1994/04/27  13:16:13  mryan]
 * 
 * 	CR 8766 - Create macros to fetch fields given the cache
 * 	[1994/04/27  13:03:33  mryan]
 * 
 * Revision 1.1.7.9  1994/04/26  21:48:23  dick
 * 	Change over to using _XmStringEntry as the segment data type
 * 	[1994/04/26  21:25:36  dick]
 * 
 * Revision 1.1.7.8  1994/04/26  17:00:05  inca
 * 	Merged with changes from 1.1.7.7
 * 	[1994/04/26  17:00:00  inca]
 * 
 * 	Make CSText understand unopt. segments
 * 	[1994/04/25  22:56:02  inca]
 * 
 * Revision 1.1.7.7  1994/04/26  15:26:50  drk
 * 	CR 8722:  Cleanup CodeCenter warnings.
 * 	[1994/04/26  15:14:19  drk]
 * 
 * Revision 1.1.7.6  1994/04/12  21:35:00  mryan
 * 	Add output line max ascent/descent fields.
 * 	[1994/04/12  21:32:15  mryan]
 * 
 * Revision 1.1.7.5  1994/04/11  21:35:17  dick
 * 	Get things setup to handle multibyte and widechar correctly
 * 	[1994/04/11  21:24:05  dick]
 * 
 * Revision 1.1.7.4  1994/03/14  16:34:40  inca
 * 	DEC drop
 * 	[1994/03/14  16:34:12  inca]
 * 
 * Revision 1.1.7.3  1994/03/10  22:29:31  inca
 * 	Merged with changes from 1.1.7.2
 * 	[1994/03/10  22:28:39  inca]
 * 
 * 	Oops, missed this one
 * 	[1994/03/10  17:40:43  inca]
 * 
 * 	Latest drop from DEC
 * 	[1994/03/10  13:46:37  inca]
 * 
 * Revision 1.1.7.2  1994/03/10  21:53:05  drk
 * 	Remove unsupported #ifdefs.
 * 	[1994/03/10  21:47:07  drk]
 * 
 * Revision 1.1.7.1  1994/03/04  22:21:38  drk
 * 	Hide _Xm routines from public view.
 * 	[1994/03/04  21:55:12  drk]
 * 
 * Revision 1.1.4.9  1993/11/12  15:53:20  dick
 * 	Change x y values in output lines to from Position to int
 * 	[1993/11/12  01:08:51  dick]
 * 
 * Revision 1.1.4.8  1993/11/11  22:28:30  inca
 * 	Merged with changes from 1.1.4.7
 * 	[1993/11/11  22:28:02  inca]
 * 
 * 	Added event to parameter list for _XmCSTextReplaceString and
 * 	call_source_[modify|changed]_callback so the callback struct
 * 	would get filled in.
 * 	[1993/11/11  20:36:08  inca]
 * 
 * Revision 1.1.4.7  1993/11/11  22:03:49  hartsing
 * 	added take_selection field to source struct for selection handling
 * 	[1993/11/11  22:03:22  hartsing]
 * 
 * Revision 1.1.4.6  1993/10/12  21:30:17  motifdec
 * 	Added the CacheNew macro back to this file until it can be moved to
 * 	XmStringI.h properly.
 * 	[1993/10/12  21:29:41  motifdec]
 * 
 * Revision 1.1.4.5  1993/10/12  21:17:36  motifdec
 * 	Cleaning up.
 * 	[1993/10/12  21:10:41  motifdec]
 * 
 * Revision 1.1.4.4  1993/09/16  18:04:26  motifdec
 * 	Fixed left-hand side HP problems.
 * 	[1993/09/16  18:02:46  motifdec]
 * 
 * Revision 1.1.4.3  1993/09/16  13:19:23  drk
 * 	Include "XmStringI.h", not <Xm/XmStringI.h>.
 * 	[1993/09/16  13:19:04  drk]
 * 
 * Revision 1.1.4.2  1993/09/16  02:29:17  motifdec
 * 	Some performance and restructuring with new XmString structs
 * 	[1993/09/16  02:15:39  motifdec]
 * 
 * Revision 1.1.2.8  1993/07/23  02:47:14  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:12:30  yak]
 * 
 * Revision 1.1.2.7  1993/07/20  17:22:43  daniel
 * 	fix sgi compiler errors
 * 	[1993/07/20  17:21:51  daniel]
 * 
 * Revision 1.1.2.6  1993/07/02  17:20:39  motifdec
 * 	Moved some source specific protos to CSTextI.h
 * 	[1993/07/02  17:13:10  motifdec]
 * 
 * Revision 1.1.2.5  1993/06/16  20:19:18  motifdec
 * 	June 16 code drop from digital
 * 	[1993/06/16  19:01:28  motifdec]
 * 
 * $EndLog$
 */

/*
 * CSTextSrcP.h
 * for Motif release 2.0
 */

#ifndef _XmCSTextSrcP_h
#define _XmCSTextSrcP_h

#include <Xm/XmP.h>
#include <Xm/CSTextP.h>
#include "XmStringI.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SEGMENT_EXTRA_ALLOCATION 20 

#define DXmLanguageNotSpecified 0
#define DXmRendMaskNone 0

/* Disposition of current selection */

typedef enum {			   
    GetSelection,
    KeepSelection,
    DisownSelection
} XmCSTextSelectionType;



/****************************************************************
 *
 * Output data structures
 *
 ****************************************************************/


/* Rendition Manager */
typedef enum {
  RmHIGHLIGHT_NORMAL = XmHIGHLIGHT_NORMAL,
  RmHIGHLIGHT_SELECTED = XmHIGHLIGHT_SELECTED,
  RmHIGHLIGHT_SECONDARY_SELECTED = XmHIGHLIGHT_SECONDARY_SELECTED,
  RmSEE_DETAIL
} RmHighlightMode;


/*
 * the source segment XmCSTextOutputSegment keeps a pointer to an 
 * array of these structs, one per char, tells us all the stuff we
 * need to know.
 */
typedef struct
{
    Boolean         need_redisplay;   /* this char is dirty */

    XmHighlightMode draw_mode;        /* how this char is to be drawn */
} XmCSTextOutputCharRec, *XmCSTextOutputChar;

/*
** Want to avoid potential conflict with additional cache types which might be
** added by OSF in the early rounds.  Once this is stable, NEW cache types (if
** not the actual structures) should move to XmStringI.h.  RJS
*/
#define _XmCSTEXT_OUT_CACHE    251

/* 
 * OutputSegment cache to store the extra OutputSegment fields.
 * These are fields specific enough to CSText that it did not make sense
 * to store in the XmString structures.
 */
typedef struct _CSTextOutSeg {
  _XmStringCacheHeader header;    /* cache_type == _XmCSTEXT_OUT_CACHE */

  /* No matching fields - does not make sense since they will not be shared */

  /* Cached data */
  int                 offset;           /* where to start in source seg */
  Boolean	      new_line;         /* indicate if segment starts at */
					/* beginning of line		 */
  RmHighlightMode     draw_mode;	/* highlight mode */
} _CSTextOutSegCacheRec, *_CSTextOutSegCache;
  
/*
 * Macros to access OutputSegment fields
 */
#define CST_TotalCharCount(w, seg) \
(_XmEntryCharCountGet(seg, CST_FontList(w))+_XmEntryTabsGet(seg))

#define CST_OutSegTotalCharCount(w, out_seg) (CST_TotalCharCount(w, out_seg))

#define CST_OutSegRenderingCache(out_seg)\
  ((_XmStringRenderingCache)\
	(_XmStringCacheGet(_XmEntryCacheGet((_XmStringEntry)out_seg),\
			   _XmRENDERING_CACHE)))
#define CST_OutSegNeedRecompute(out_seg)\
        (CST_OutSegRenderingCache(out_seg)->header.dirty)
#define CST_OutSegX(out_seg) (CST_OutSegRenderingCache(out_seg)->x)
#define CST_OutSegY(out_seg) (CST_OutSegRenderingCache(out_seg)->y)
#define CST_OutSegWidth(out_seg) (CST_OutSegRenderingCache(out_seg)->width)
#define CST_OutSegHeight(out_seg) (CST_OutSegRenderingCache(out_seg)->height)
#define CST_OutSegAscent(out_seg) (CST_OutSegRenderingCache(out_seg)->ascent)
#define CST_OutSegDescent(out_seg) (CST_OutSegRenderingCache(out_seg)->descent)

#define CST_OutSegFontEntry(out_seg) (CST_OutSegRenderingCache(out_seg)->rendition)

#define CST_OutSegCache(out_seg)\
  ((_CSTextOutSegCache)\
	(_XmStringCacheGet(_XmEntryCacheGet((_XmStringEntry)out_seg),\
			   _XmCSTEXT_OUT_CACHE)))
#define CST_OutSegOffset(out_seg) (CST_OutSegCache(out_seg)->offset)
#define CST_OutSegNeedRedisplay(out_seg) (CST_OutSegCache(out_seg)->header.dirty)
#define CST_OutSegNewLine(out_seg) (CST_OutSegCache(out_seg)->new_line)
#define CST_OutSegDrawMode(out_seg) (CST_OutSegCache(out_seg)->draw_mode)

/* Alternate versions of the above macros, which take the cache as an
 * argument rather than the out_seg (cache searches are expensive, so
 * smart functions will get the cache and pass it to these macros, rather
 * than using the above macros and taking the cache search hit every
 * time)
 */
#define CST_OutSegRCacheNeedRecompute(rcache) ((rcache)->header.dirty)
#define CST_OutSegRCacheX(rcache) ((rcache)->x)
#define CST_OutSegRCacheY(rcache) ((rcache)->y)
#define CST_OutSegRCacheWidth(rcache) ((rcache)->width)
#define CST_OutSegRCacheHeight(rcache) ((rcache)->height)
#define CST_OutSegRCacheAscent(rcache) ((rcache)->ascent)
#define CST_OutSegRCacheDescent(rcache) ((rcache)->descent)
#define CST_OutSegRCacheFontEntry(rcache) ((rcache)->rendition)

#define CST_OutSegCacheOffset(cache) ((cache)->offset)
#define CST_OutSegCacheNeedRedisplay(cache) ((cache)->header.dirty)
#define CST_OutSegCacheNewLine(cache) ((cache)->new_line)
#define CST_OutSegCacheDrawMode(cache) ((cache)->draw_mode)

/* 
 * the source segment keeps a pointer to one of these structs for
 * us.  keep all the line level data here
 */
typedef struct _XmCSTextOutputLineRec
{
    Boolean             need_redisplay,   /* segment in the line is dirty */
                        need_recompute;   /* something is know to be wrong */

    short               pixel_width,      /* size of this line */
                        pixel_height;

    int		        x, y;		  /* location of this line in window */
    Dimension           max_ascent,       /* Maximum ascent/descent of all */
    			max_descent;	  /* out_segs in the line */
    int			char_count;	  /* num of char in this out_line */
    RmHighlightMode	draw_mode;	  /* highlight mode */
} XmCSTextOutputLineRec;



/****************************************************************
 *
 * Source data structures
 *
 ****************************************************************/

typedef Atom XmCSTextFormat;

typedef struct _CSTextOutputLineDataRec {
    XmCSTextOutputLine*	output_lines;
    Cardinal		output_count;
} CSTextOutputLineDataRec, *CSTextOutputLineData;

typedef struct _CSTextOutputSegmentDataRec {
    _XmStringEntry     *output_segments;
    Cardinal		   output_count;
} CSTextOutputSegmentDataRec, *CSTextOutputSegmentData;


/* 
 * SourceSegment cache to store the extra SourceSegment fields.
 * These are fields specific enough to CSText that it did not make sense
 * to store in the XmString structures.
 */
typedef struct _CSTextSourceSeg {
  _XmStringCacheHeader header;    /* cache_type == _XmCSTEXT_CACHE */

  /* No matching fields - does not make sense since they will not be shared */

  /* Cached data */
    int			allocation; 	/* segment length in bytes (total)   */
    XmStringCharSet	charset;    	/* char set name string		     */
    CSTextOutputSegmentData output_segments; /* array of per widget data     */
} _CSTextSourceSegCacheRec, *_CSTextSourceSegCache;


/*
 * Macros to access SourceSegment structures
 */

/*
 * #define SourceSegLocale - do we need locale any more?
 */
#define CST_SourceSegTotalCharCount(w,seg) (CST_TotalCharCount(w,seg))

#define CST_SourceSegScanningCache(seg)\
  ((_XmStringScanningCache)\
	(_XmStringCacheGet(_XmEntryCacheGet((_XmStringEntry)seg),\
			   _XmSCANNING_CACHE)))
#define CST_SourceSegNext(seg) (CST_SourceSegScanningCache(seg)->right)
#define CST_SourceSegPrev(seg) (CST_SourceSegScanningCache(seg)->left)
#define CST_SourceSegDirection(seg) (CST_SourceSegScanningCache(seg)->layout_direction)
#define CST_SourceSegNestLevel(seg) (CST_SourceSegScanningCache(seg)->depth)

#define CST_SourceSegCache(seg)\
  ((_CSTextSourceSegCache)\
   (_XmStringCacheGet(_XmEntryCacheGet((_XmStringEntry)seg),_XmCSTEXT_CACHE)))
#define CST_SourceSegBytesUsed(seg) (CST_SourceSegCache(seg)->used_allocation)
#define CST_SourceSegBytesTotal(seg) (CST_SourceSegCache(seg)->allocation)
#define CST_SourceSegCharset(seg) (CST_SourceSegCache(seg)->charset)
#define CST_SourceSegLocale(seg) (CST_SourceSegCache(seg)->locale)
#define CST_SourceSegWidgetOutSegArray(seg)\
        (CST_SourceSegCache(seg)->output_segments)
/*
** Convenience macros for allocating segment cache records.
*/
#define CSTextSegCacheScanningNew(p) \
  { \
      p = (_XmStringScanningCache)XtCalloc(1,sizeof(_XmStringScanningRec));\
      p->prim_dir = XmDEFAULT_DIRECTION; \
      p->layout_direction = XmDEFAULT_DIRECTION; \
      p->header.cache_type = _XmSCANNING_CACHE; \
  }

#define CSTextSegCacheRenderingNew(p) \
  { \
      p = (_XmStringRenderingCache)XtCalloc(1,sizeof(_XmStringRenderingRec));\
      p->header.dirty = True; \
      p->header.cache_type = _XmRENDERING_CACHE; \
  }

#define CSTextSegCacheHighlightNew(p) \
  { \
      p = (_XmStringHighlightCache)XtCalloc(1,sizeof(_XmStringHighlightCacheRec));\
      p->header.cache_type = _XmHIGHLIGHT_CACHE; \
  }

#define CSTextSegCacheCSTextNew(p) \
  { \
      p = (_CSTextSourceSegCache)XtCalloc(1,sizeof(_CSTextSourceSegCacheRec));\
      p->header.cache_type = _XmCSTEXT_CACHE; \
  }

#define CSTextSegCacheCSTextOutNew(p) \
  { \
      p = (_CSTextOutSegCache)XtCalloc(1,sizeof(_CSTextOutSegCacheRec));\
      p->header.dirty = True;\
      p->new_line = False;\
      p->draw_mode = RmHIGHLIGHT_NORMAL;\
      p->header.cache_type = _XmCSTEXT_OUT_CACHE; \
  }

#define CSTextSegCacheAttach(p, n) \
  { \
      _XmCacheNext(p) = (_XmStringCache)n;\
  }

typedef struct _CSTextLine {
    struct _CSTextLine	  *next;	   /* next text line		   */
    struct _CSTextLine	  *prev;	   /* prev text line		   */
    _XmStringEntry	  segments;	   /* Ptr to 1st text segment in   */
					   /* line			   */
    XmCSTextSource	  source;	   /* Back ptr to source record    */
    CSTextOutputLineData output_data;	   /* Ptr to output data	   */
    char		  *value;	   /* Transfer buffer		   */
    XmTextPosition	  length;	   /* length of line in characters */ 
    int			  maxallowed;	   /* Maximum source line may grow */
    Boolean		  editable;	   /* Line may be edited	   */

  } XmCSTextLineRec;

typedef struct _XmCSTextSourceData {
    XmCSTextWidget *widgets;	      /* Array of widgets displaying this */
    				      /* source  */
    int            num_widgets;	      /* Number of entries in above	    */
    XmCSTextLine   source_lines;      /* Ptr. to first source line	    */
    int            length;            /* Number of characters in source   */
    Time	   prime_time;	      /* Time of primary selection        */
    Boolean	   hasselection;      /* Whether we own selection         */
    XmTextPosition left;	      /* Left extent of selection	    */
    XmTextPosition right;	      /* Right extent of selection        */
    Boolean        take_selection;    /* Whether we should take the selection*/
  } XmCSTextSourceDataRec, *XmCSTextSourceData;


/* Function declarations for XmCSTextSource functions */

#ifdef _NO_PROTO
typedef void (*XmCSTextAddWidgetProc)();
    /* XmCSTextSource source; */
    /* Text data; */

typedef int (*XmCSTextCountLinesProc)();
    /* XmCSTextSource source; */
    /* XmTextPosition start; */
    /* int length; */

typedef void (*XmCSTextRemoveWidgetProc)();
    /* XmCSTextSource source; */
    /* Text data; */

typedef XmTextPosition (*XmCSTextReadProc)();
    /* XmCSTextSource source; */
    /* XmTextPosition pos; */	/* starting position */
    /* XmTextPosition lastPos; */ /* The last position we're interested in.
       				     Don't return info about any later
				     positions. */
    /* XmTextBlock block; */	/* RETURN: text read in */

typedef XmTextStatus (*XmCSTextReplaceProc)();
    /* XmCSTextSource source; */
    /* XmTextPosition start, end; */
    /* XmTextBlock block; */

typedef XmTextPosition (*XmCSTextScanProc)();
    /* XmCSTextSource source; */
    /* XmTextPosition pos; */
    /* XmCSTextScanType sType; */
    /* XmTextScanDirection dir; */  /* Either XmsdLeft or XmsdRight. */
    /* int n; */
    /* Boolean include; */

typedef Boolean (*XmCSTextGetSelectionProc)();
    /* XmCSTextSource source; */
    /* XmTextPosition *left, *right; */ 

typedef void (*XmCSTextSetSelectionProc)();
    /* XmCSTextSource source; */
    /* XmTextPosition left, right; */
    /* Time time; */

#else /* _NO_PROTO */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*XmCSTextAddWidgetProc)(
	XmCSTextSource source, XmCSTextWidget data
    );
typedef int (*XmCSTextCountLinesProc)(
	XmCSTextSource source, XmTextPosition start, int length
    );
typedef void (*XmCSTextRemoveWidgetProc)(
	XmCSTextSource source, XmCSTextWidget data
    );
typedef XmTextPosition (*XmCSTextReadProc)(
	XmCSTextSource source, XmTextPosition pos, XmTextPosition lastPos,
	XmTextBlock block
    );
typedef XmTextStatus (*XmCSTextReplaceProc)(
	XmCSTextSource source, XEvent *event,
	XmTextPosition start, XmTextPosition end,
	XmTextBlock block
    );
typedef XmTextPosition (*XmCSTextScanProc)(
	XmCSTextSource source,
	XmTextPosition pos,
	XmTextScanType sType,
	XmTextScanDirection dir,
	int n,
#if NeedWidePrototypes
	int include
#else
	Boolean include
#endif
    );
typedef Boolean (*XmCSTextGetSelectionProc)(
	XmCSTextSource source,
	XmTextPosition *left,
	XmTextPosition *right
    );
typedef void (*XmCSTextSetSelectionProc)(
	XmCSTextSource source,
	XmTextPosition left,
	XmTextPosition right,
	Time time
    );

#ifdef __cplusplus
};
#endif
#endif /* else _NO_PROTO */
  
 
typedef struct _XmCSTextSourceRec {
    XmCSTextSourceData		data;   /* Source-defined data (opaque) */
    XmCSTextAddWidgetProc	AddWidget;
    XmCSTextCountLinesProc	CountLines;
    XmCSTextRemoveWidgetProc	RemoveWidget;
    XmCSTextReadProc		ReadSource;
    XmCSTextReplaceProc		Replace;
    XmCSTextScanProc		Scan;
    XmCSTextGetSelectionProc	GetSelection;
    XmCSTextSetSelectionProc	SetSelection;
} XmCSTextSourceRec;
    
  
/* data structure for holding source location information
*/
typedef struct _CSTextLocation {
    XmTextPosition	position;    /* linear logical position in source  */
    XmCSTextLine	line;        /* source line of position            */
    XmTextPosition	line_offset; /* text offset in line of position    */
    _XmStringEntry	segment;     /* source segment of position         */
    XmTextPosition 	offset;      /* text offset in segment of position */
    Boolean          	end_of_line; /* is position at the end of a line?  */
  } CSTextLocationRec, *CSTextLocation;

typedef struct _XmCSTextCvtContextRec *XmCSTextCvtContext;
#ifdef _NO_PROTO
typedef void (*XmCSTextSegmentProc)();
#else
typedef void (*XmCSTextSegmentProc)(XmCSTextCvtContext);
#endif
 
/*
 * data struct shared by CSText source and DXmCvtCStoCSText and DXmCvtCSTextToCS
 * routines
 */

typedef struct _XmCSTextCvtContextRec
{
    /* 
     * fields used for communication between the caller and callee,
     *  **** DON'T TOUCH THIS DATA STRUCTURE *** cause XmString.c must match
     */

    XmTextStatus    	status;	      /* return status from converter      */

    XmCSTextSegmentProc	line_cb;      /* ptr to proc to call when new line is*/
                                      /* found during conversion             */
    XmCSTextSegmentProc segment_cb;   /* ptr to proc to call when segment is */
                                      /* found during conversion             */

                                      /* info about sement being converted   */
    char	      	*text;	      /* the next string of characters       */
    int		      	byte_length;  /* length of the string in chars       */
    XmStringCharSet   	charset;      /* character set of the string         */
    XmStringDirection 	direction;    /* direction of the string             */
    short	      	locale;	      /* the locale of the string	     */
    int		      	nest_level;   /* nesting level of this segment       */

    XmString          	stream;       /* result of CSText -> CS conversion   */

    /* 
     * fields private to the CSText source during conversion
     */
    Widget		widget;	      /* the widget id of the CSText widget  */
    XmCSTextLine	line;	      /* address of line for use by callback */
    _XmStringEntry	segment;      /* address of segment used by callback */
    XmTextPosition  	offset;	      /* offset in segment used by callback */
    short		alloc_extra;  /* extra anticipatory allocation       */
    short	      	initialized;  /* has the convert been initialized    */
    XmFontListEntry	fontlist_entry;
    XmTextType		text_type;

    /*
     * fields private to the compound string converter
     */

    XmString		*answer;      	/* array of entries made during cvt */
    int               	answers;
    XmStringDirection	prev_direction;
    Boolean           	emitted_extra;	/* you don't want to know... */
}   
    XmCSTextCvtContextRec;


#ifdef __cplusplus

}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif
 
#endif /*  _XmCSTextSrcP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
