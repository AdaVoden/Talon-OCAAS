/* $XConsortium: IconFileP.h /main/5 1995/07/15 20:51:48 drk $ */
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
#ifndef _XmIconFileP_h
#define _XmIconFileP_h

#include <Xm/IconFile.h>
#include <Xm/XmosP.h>

#ifdef __cplusplus
extern "C" {
#endif


/********    Private Function Declarations for IconFile.c    ********/

extern void XmeFlushIconFileCache(String	path);

/********    End Private Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmIconFile_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */



