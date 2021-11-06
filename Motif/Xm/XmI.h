/* 
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
*/ 
/* 
 * HISTORY
 * $Log: XmI.h,v $
 * Revision 1.1.1.1  2003/04/15 20:47:54  lostairman
 * Initial Import
 *
 * Revision 1.1.1.1  2001/04/19 21:12:08  ocaas
 * Initial migration to CVS of OCAAS 2.0.11a
 *
 * Revision 1.7.176.2  1995/11/15  22:42:47  dbl
 * 	unnumbered CDE convergence: _XmCreateImage and _XmDestroyParentCallback for 1.2 BC
 * 	[1995/11/15  22:40:41  dbl]
 *
 * Revision 1.7.176.1  1995/11/14  22:25:53  dbl
 * 	CR 10291: Motif.next convergence: XmNenableThinThickness [1.2.5 regression]
 * 	[1995/11/14  22:24:52  dbl]
 * 
 * Revision 1.7.172.2  1994/06/29  20:41:20  drk
 * 	CR 9539: Fix SGI 64-bit warnings.
 * 	[1994/06/29  20:13:05  drk]
 * 
 * Revision 1.7.172.1  1994/06/02  22:30:36  inca
 * 	Corrected a typo
 * 	[1994/06/02  18:13:36  inca]
 * 
 * 	Move direction macros and function defs to XmI.h
 * 	[1994/06/02  18:01:59  inca]
 * 
 * Revision 1.7.170.11  1994/03/29  22:20:20  meeks
 * 	PERF: Removed XmStackRealloc to function in XmString.c.
 * 	[1994/03/29  21:59:03  meeks]
 * 
 * Revision 1.7.170.10  1994/03/25  16:26:41  deblois
 * 	Added prototype for _XmGetDefaultDisplay.
 * 	[1994/03/25  16:26:22  deblois]
 * 
 * Revision 1.7.170.9  1994/03/22  14:59:48  meeks
 * 	Fixed compiler warnings.
 * 	[1994/03/21  19:44:26  meeks]
 * 
 * 	PERF: Added XmStackRealloc.
 * 	[1994/03/21  18:11:16  meeks]
 * 
 * Revision 1.7.170.8  1994/03/10  21:54:04  drk
 * 	Remove unsupported #ifdefs.
 * 	[1994/03/10  21:48:05  drk]
 * 
 * Revision 1.7.170.7  1994/03/09  23:29:07  drk
 * 	CR 6404: Include <limits.h> to suppress HP warnings about MIN/MAX.
 * 	[1994/03/09  23:28:49  drk]
 * 
 * Revision 1.7.170.6  1994/03/07  20:26:33  deblois
 * 	Added _XmValidTimestamp.
 * 	[1994/03/07  20:26:15  deblois]
 * 
 * Revision 1.7.170.5  1994/03/04  22:28:50  drk
 * 	Hide _Xm routines from public view.
 * 	[1994/03/04  22:02:11  drk]
 * 
 * Revision 1.7.170.4  1994/03/01  22:27:46  drk
 * 	Merged with changes from 1.7.170.3
 * 	[1994/03/01  22:27:29  drk]
 * 
 * 	Promote _XmParseUnits, obsolesce _XmGrabTheFocus.
 * 	[1994/03/01  19:01:08  drk]
 * 
 * Revision 1.7.170.3  1994/03/01  22:01:18  meeks
 * 	#include XmRenderTI.h, XmTabListI.h.
 * 	[1994/03/01  21:59:34  meeks]
 * 
 * Revision 1.7.170.2  1994/02/24  23:41:16  drk
 * 	Remove _Xm and Xt functions from XmP.h
 * 	[1994/02/24  23:37:37  drk]
 * 
 * Revision 1.7.170.1  1994/01/27  22:32:35  drk
 * 	CR 6149: Allow -DANSICPP for partially conformant preprocessors.
 * 	[1994/01/27  21:39:11  drk]
 * 
 * Revision 1.7.9.9  1993/10/26  21:18:32  drk
 * 	Rationalize context state enumeration order.
 * 	[1993/10/26  20:42:56  drk]
 * 
 * Revision 1.7.9.8  1993/10/19  21:13:34  meeks
 * 	I 160:  Added fields and macros for groundStates in XmRendition.
 * 	[1993/10/19  21:02:19  meeks]
 * 
 * Revision 1.7.9.7  1993/10/13  21:29:04  drk
 * 	Fixup tab handling in optimized strings.
 * 	[1993/10/13  21:18:36  drk]
 * 
 * Revision 1.7.9.6  1993/10/12  21:10:27  meeks
 * 	Added hadEnds to XmRendition.
 * 	[1993/10/12  20:43:16  meeks]
 * 
 * Revision 1.7.9.5  1993/09/16  20:29:07  dbrooks
 * 	Undo previous submission.
 * 	Fixes from Motif1.2.4. Rev. 1.7.15.2, ancestor 1.7.13.2, file XmI.h.
 * 	[1993/09/16  20:28:50  dbrooks]
 * 
 * Revision 1.7.9.4  1993/09/16  19:51:28  dbrooks
 * 	Rationalize MAX/MIN macros on HP-UX.  CR 6404.
 * 	Fixes from Motif1.2.4. Rev. 1.7.12.2, ancestor 1.7.2.2, file XmI.h.
 * 	[1993/09/16  19:51:09  dbrooks]
 * 
 * Revision 1.7.9.3  1993/09/14  20:01:41  drand
 * 	Added _XmGetEncodingRegistryTarget
 * 	[1993/09/14  20:01:20  drand]
 * 
 * Revision 1.7.9.2  1993/09/14  17:24:10  meeks
 * 	Removed XmString declarations that are duplicated in XmStringI.h.
 * 	[1993/09/14  15:49:45  meeks]
 * 
 * Revision 1.7.5.30  1993/07/23  03:06:17  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:40:55  yak]
 * 
 * Revision 1.7.5.29  1993/07/20  18:12:43  meeks
 * 	CR 6055: Added mark bit to render tables.
 * 	[1993/07/20  18:12:22  meeks]
 * 
 * Revision 1.7.5.28  1993/07/10  02:28:07  meeks
 * 	2.0 4.2.2: Handles for render tables.
 * 	[1993/07/10  02:09:32  meeks]
 * 
 * Revision 1.7.5.27  1993/07/08  18:26:30  drk
 * 	Work around bogus Sun "operands of : have incompatible types" errors.
 * 	[1993/07/08  13:51:31  drk]
 * 
 * Revision 1.7.5.26  1993/07/01  01:07:04  meeks
 * 	2.0 4.2: Out of sync with XmString.c.  Attempting to resynchronize.
 * 	Added display field to render table.
 * 	[1993/07/01  01:05:19  meeks]
 * 
 * Revision 1.7.5.25  1993/06/30  22:53:43  drk
 * 	Made _XmParseMappingRec an internal type.
 * 	[1993/06/30  22:50:16  drk]
 * 
 * Revision 1.7.5.24  1993/06/29  22:35:17  drk
 * 	Cleanup warnings in assert().
 * 	[1993/06/29  22:28:09  drk]
 * 
 * Revision 1.7.5.23  1993/06/25  15:49:52  meeks
 * 	2.0 4.2.1: Added display field to tabs.
 * 	[1993/06/25  15:48:26  meeks]
 * 
 * Revision 1.7.5.22  1993/06/24  22:52:29  inca
 * 	Added support for LAYOUT_PUSH and LAYOUT_POP components
 * 	[1993/06/24  17:31:56  inca]
 * 
 * Revision 1.7.5.21  1993/06/15  22:15:55  meeks
 * 	2.0 4.2.1, 4.2.2: Moved _XmTab macros here and changed signature of
 * 	_XmStringDrawSegment.
 * 	[1993/06/15  22:01:44  meeks]
 * 
 * Revision 1.7.5.20  1993/06/11  21:01:57  drk
 * 	Added tag_type to string context iterators.
 * 	[1993/06/11  20:58:11  drk]
 * 
 * Revision 1.7.5.19  1993/06/11  19:33:04  meeks
 * 	2.0 4.2.2: Changed signature of _XmStringDrawSegment.
 * 	[1993/06/11  14:50:54  meeks]
 * 
 * Revision 1.7.5.18  1993/06/11  00:13:52  drk
 * 	Cleanup assert() macro for pedantic compilers, reorder fields in
 * 	_XmStringSegmentRec to improve layout.
 * 	[1993/06/11  00:11:38  drk]
 * 
 * 	Deleted unused types; fixed string context to handle renditions and
 * 	tabs.
 * 	[1993/06/09  21:53:35  drk]
 * 
 * Revision 1.7.5.17  1993/06/08  23:15:13  meeks
 * 	2.0 4.2.2: Data structure changes.
 * 	[1993/06/08  22:43:34  meeks]
 * 
 * Revision 1.7.5.16  1993/06/08  22:57:12  drk
 * 	Updated to account for recent XmStringParseText spec changes.
 * 	[1993/06/08  22:54:29  drk]
 * 
 * Revision 1.7.5.15  1993/06/07  13:43:29  meeks
 * 	Fixed error in editing for merge.
 * 	[1993/06/07  13:43:08  meeks]
 * 
 * Revision 1.7.5.14  1993/06/07  01:44:36  meeks
 * 	2.0 4.2.1, 4.2.2: Moved rendition macros from .c to here so that
 * 	XmString could access.  Added tabs to XmStringSegment.  Modified XmTab
 * 	and XmTabList structures.
 * 	[1993/06/07  01:39:36  meeks]
 * 
 * Revision 1.7.5.13  1993/06/02  21:58:12  drk
 * 	Renamed XmStringIncludeStatus to be XmIncludeStatus.
 * 	[1993/06/02  18:54:17  drk]
 * 
 * Revision 1.7.5.12  1993/05/27  21:59:36  drk
 * 	Fix typo in _XmSegRendTagGet, added _XmSegRendTags.
 * 	[1993/05/27  21:58:11  drk]
 * 
 * Revision 1.7.5.11  1993/05/25  13:41:24  drk
 * 	Cleanup comment wording.
 * 	[1993/05/25  13:41:05  drk]
 * 
 * Revision 1.7.5.10  1993/05/24  18:36:11  meeks
 * 	2.0 4.2.2: Modifications for XmFontList conversion.
 * 	[1993/05/24  18:19:05  meeks]
 * 
 * Revision 1.7.5.9  1993/05/19  20:12:06  meeks
 * 	2.0: Modifications for XmString and XmRenderTable.
 * 	[1993/05/19  20:04:42  meeks]
 * 
 * Revision 1.7.5.8  1993/05/14  21:36:47  drk
 * 	Added XmParseProc type & constant.
 * 	[1993/05/14  21:34:55  drk]
 * 
 * Revision 1.7.5.7  1993/05/12  19:11:47  meeks
 * 	2.0 4.2: Added structure declarations.
 * 	[1993/05/12  18:50:59  meeks]
 * 
 * Revision 1.7.5.6  1993/04/27  14:06:25  rgcote
 * 	Added an "int ignore" field to the _XmRenditionRec struct
 * 	to make the HP compiler stop complaining.
 * 	[1993/04/27  13:19:53  rgcote]
 * 
 * Revision 1.7.5.5  1993/04/26  22:39:12  meeks
 * 	2.0 4.2.5: Changed typedef of XmStringContext.
 * 	[1993/04/26  22:29:58  meeks]
 * 
 * Revision 1.7.5.4  1993/04/19  15:04:14  drk
 * 	Included <stdio.h> to pick up fprintf(), stderr.
 * 	[1993/04/19  15:03:40  drk]
 * 
 * Revision 1.7.5.3  1993/04/16  22:01:26  drk
 * 	Created an assert() macro.
 * 	[1993/04/16  22:00:10  drk]
 * 
 * Revision 1.7.5.2  1993/02/19  19:52:38  meeks
 * 	Added FontList info to be shared between XmString.c and XmFontList.c.
 * 	[1993/02/19  19:45:51  meeks]
 * 
 * Revision 1.7.2.2  1992/03/25  19:16:22  daniel
 * 	Integrate HP diffs
 * 	[1992/03/25  18:33:17  daniel]
 * 
 * Revision 1.7  1992/03/13  16:56:06  devsrc
 * 	Converted to ODE
 * 
 * $EndLog$
*/ 
/*   $RCSfile: XmI.h,v $ $Revision: 1.1.1.1 $ $Date: 2003/04/15 20:47:54 $ */
/* (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmI_h
#define _XmI_h

#ifndef _XmNO_BC_INCL
#define _XmNO_BC_INCL
#endif

#include <stdio.h>
#include <limits.h>
#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEBUG
# define assert(assert_exp)
#elif (defined(__STDC__) && __STDC__ && !defined(UNIXCPP)) || defined(ANSICPP)
# define assert(assert_exp)						\
  (((assert_exp) ? (void) 0 :						\
    (void) (fprintf(stderr, "assert(%s) failed at line %d in %s\n",	\
                    #assert_exp, __LINE__, __FILE__), abort())))
#else
# define assert(assert_exp)						\
  (((assert_exp) ? 0 :							\
    (void) (fprintf(stderr, "assert(%s) failed at line %d in %s\n",	\
		    "assert_exp", __LINE__, __FILE__), abort())))
#endif


#define ASSIGN_MAX(x,y)		if ((y) > (x)) x = (y)
#define ASSIGN_MIN(x,y)		if ((y) < (x)) x = (y)

#ifndef MAX
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y)	((x) > (y) ? (y) : (x))
#endif

#ifndef ABS
#define ABS(x)		(((x) >= 0) ? (x) : -(x))
#endif

#define GMode(g)	    ((g)->request_mode)
#define IsX(g)		    (GMode (g) & CWX)
#define IsY(g)		    (GMode (g) & CWY)
#define IsWidth(g)	    (GMode (g) & CWWidth)
#define IsHeight(g)	    (GMode (g) & CWHeight)
#define IsBorder(g)	    (GMode (g) & CWBorderWidth)
#define IsWidthHeight(g)    (GMode (g) & (CWWidth | CWHeight))
#define IsQueryOnly(g)      (GMode (g) & XtCWQueryOnly)

#define XmStrlen(s)      ((s) ? strlen(s) : 0)


#define XmStackAlloc(size, stack_cache_array)	\
    ((((char*)(stack_cache_array) != NULL) &&	\
     ((size) <= sizeof(stack_cache_array)))	\
     ?  (char *)(stack_cache_array)		\
     :  XtMalloc((unsigned)(size)))

#define XmStackFree(pointer, stack_cache_array) \
    if ((pointer) != ((char*)(stack_cache_array))) XtFree(pointer);


/******** _XmCreateImage ********/

#ifdef NO_XM_1_2_BC

/* The _XmCreateImage macro is used to create XImage with client
   specific data for the bit and byte order.
   We still have to do the following because XCreateImage
   will stuff here display specific data and we want 
   client specific values (i.e the bit orders we used for 
   creating the bitmap data in Motif) -- BUG 4262 */
/* Used in Motif 1.2 in DragIcon.c, MessageB.c, ReadImage.c and
   ImageCache.c */

#define _XmCreateImage(IMAGE, DISPLAY, DATA, WIDTH, HEIGHT, BYTE_ORDER) {\
    IMAGE = XCreateImage(DISPLAY,\
			 DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)),\
			 1,\
			 XYBitmap,\
			 0,\
			 DATA,\
			 WIDTH, HEIGHT,\
			 8,\
			 (WIDTH+7) >> 3);\
    IMAGE->byte_order = BYTE_ORDER;\
    IMAGE->bitmap_unit = 8;\
    IMAGE->bitmap_bit_order = LSBFirst;\
}

#endif /* NO_XM_1_2_BC */


/****************************************************************
 *
 *  Macros for Right-to-left Layout
 *
 ****************************************************************/

#define GetLayout(w)     (_XmGetLayoutDirection((Widget)(w)))
#define LayoutM(w)       (XmIsManager(w) ? \
			  ((XmManagerWidget)w)->manager.string_direction : \
			  GetLayout(w))
#define LayoutP(w)       (XmIsPrimitive(w) ? \
			  ((XmPrimitiveWidget)w)->primitive.layout_direction :\
			  GetLayout(w))
#define LayoutG(w)       (XmIsGadget(w) ? \
			  ((XmGadget)w)->gadget.layout_direction : \
			  GetLayout(w))

#define LayoutIsRtoL(w)      \
  (XmDirectionMatchPartial(GetLayout(w), XmRIGHT_TO_LEFT, XmHORIZONTAL_MASK))
#define LayoutIsRtoLM(w)     \
  (XmDirectionMatchPartial(LayoutM(w), XmRIGHT_TO_LEFT, XmHORIZONTAL_MASK))
#define LayoutIsRtoLP(w)     \
  (XmDirectionMatchPartial(LayoutP(w), XmRIGHT_TO_LEFT, XmHORIZONTAL_MASK))
#define LayoutIsRtoLG(w)     \
  (XmDirectionMatchPartial(LayoutG(w), XmRIGHT_TO_LEFT, XmHORIZONTAL_MASK))


/********    Private Function Declarations for Direction.c    ********/
#ifdef _NO_PROTO

extern void _XmDirectionDefault();
extern void _XmFromLayoutDirection();
extern XmImportOperator _XmToLayoutDirection();
extern XmDirection _XmGetLayoutDirection() ;

#else

extern void _XmDirectionDefault(Widget widget,
  			        int offset,
  			        XrmValue *value );
extern void _XmFromLayoutDirection( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;

extern XmImportOperator _XmToLayoutDirection( 
                        Widget widget,
                        int offset,
                        XtArgVal *value) ;
extern XmDirection _XmGetLayoutDirection(Widget w);

#endif

/********    Private Function Declarations for thickness  ********/
#ifdef _NO_PROTO

extern void _XmSetThickness() ;
extern void _XmSetThicknessDefault0() ;

#else
extern void _XmSetThickness( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
extern void _XmSetThicknessDefault0( 
                        Widget widget,
                        int offset,
                        XrmValue *value) ;
#endif /* _NO_PROTO */

/********    Private Function Declarations for Xm.c    ********/
#ifdef _NO_PROTO

extern void _XmReOrderResourceList() ;
extern void _XmSocorro() ;
extern Boolean _XmParentProcess() ;
extern void _XmClearShadowType() ;
#ifdef NO_XM_1_2_BC
extern void _XmDestroyParentCallback() ;
#endif
extern Time _XmValidTimestamp() ;
extern void _XmWarningMsg();
extern Display *_XmGetDefaultDisplay();
 
#else

extern void _XmReOrderResourceList( 
			WidgetClass widget_class,
			String res_name,
                        String insert_after) ;
extern void _XmSocorro( 
                        Widget w,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
extern Boolean _XmParentProcess( 
                        Widget widget,
                        XmParentProcessData data) ;
extern void _XmClearShadowType( 
                        Widget w,
#if NeedWidePrototypes
                        int old_width,
                        int old_height,
                        int old_shadow_thickness,
                        int old_highlight_thickness) ;
#else
                        Dimension old_width,
                        Dimension old_height,
                        Dimension old_shadow_thickness,
                        Dimension old_highlight_thickness) ;
#endif /* NeedWidePrototypes */
#ifdef NO_XM_1_2_BC
extern void _XmDestroyParentCallback( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
#endif
extern Time _XmValidTimestamp(
			Widget w);
extern void _XmWarningMsg(Widget w,
                          char *type,
			  char *message,
			  char **params,
			  Cardinal num_params);
extern Display *_XmGetDefaultDisplay(void);

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmI_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
