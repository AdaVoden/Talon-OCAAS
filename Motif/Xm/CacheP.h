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
/*   $XConsortium: CacheP.h /main/11 1995/07/14 10:12:51 drk $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmCacheP_h
#define _XmCacheP_h

#include <Xm/GadgetP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A few convenience macros */

#define ClassCacheHead(cp)	((cp)->cache_head)
#define ClassCacheCopy(cp)	((cp)->cache_copy)
#define ClassCacheCompare(cp)	((cp)->cache_compare)
#define CacheDataPtr(p)		((XtPointer)&((XmGadgetCacheRef*)p)->data)
#define DataToGadgetCache(p)	((char*)p - XtOffsetOf(XmGadgetCacheRef,data))


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCacheP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
