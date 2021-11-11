/* xdaliclock - a melting digital clock
 * Copyright (c) 1991, 1992, 1993, 1994, 1995
 *  Jamie Zawinski <jwz@netscape.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>

#if (XtSpecificationRelease > 3)
# include <X11/Xfuncs.h>
#endif

#include "xdaliclock.h"

#ifdef SHAPE
extern int do_shape;
#endif

int no_writable_cells;


void
allocate_colors (dpy, visual, cmap,
		 fg_name, bg_name, bd_name,
		 fg_color, bg_color, bd_color)
     Display *dpy;
     Visual *visual;
     Colormap cmap;
     char *fg_name, *bg_name, *bd_name;
     XColor *fg_color, *bg_color, *bd_color;
{
  Screen *screen = DefaultScreenOfDisplay (dpy);

  if (! fg_name) fg_name = "black";
  if (! bg_name) bg_name = "white";

  no_writable_cells = 0;

#if 0
 AGAIN:
#endif

  if (do_cycle)
    {
      int visual_class = get_visual_class (dpy, visual);
      unsigned long plane_masks;
      unsigned long pixels [2];

      if (visual_class == StaticGray ||
	  visual_class == StaticColor ||
	  visual_class == TrueColor)
	{
#if 0
	  fprintf (stderr,
		   (!root_p
		    ? "%s: -cycle is incompatible with visual\n\t"
	: "%s: -cycle is incompatible with the root window's visual\n\t"),
		   progname);
	  describe_visual (stderr, dpy, visual);
	  do_cycle = 0;
	  goto AGAIN;
#else
	  no_writable_cells = 1;
	  goto NONWRITABLE;
#endif
	}
      else if (XAllocColorCells (dpy, cmap, False,
				 &plane_masks, 0, pixels, 2))
	{
	  fg_color->pixel = pixels [0];
	  bg_color->pixel = pixels [1];
	  XParseColor (dpy, cmap, fg_name, fg_color);
	  XParseColor (dpy, cmap, bg_name, bg_color);
	  fg_color->flags = bg_color->flags = DoRed | DoGreen | DoBlue;
	  XStoreColor (dpy, cmap, fg_color);
	  XStoreColor (dpy, cmap, bg_color);
	  no_writable_cells = 0;
	}
      else
	{
#if 0
	  fprintf (stderr,
	      "%s: couldn't allocate two read-write color cells on visual\n\t",
		   progname);
	  describe_visual (stderr, dpy, visual);
	  do_cycle = 0;
	  goto AGAIN;
#else
	  no_writable_cells = 1;
	  goto NONWRITABLE;
#endif
	}
    }
  else
    {
    NONWRITABLE:
      no_writable_cells = 1;
      if (! XParseColor (dpy, cmap, fg_name, fg_color))
	{
	  fprintf (stderr, "%s: can't parse color %s; using black\n",
		   progname, fg_name);
	  fg_color->pixel = BlackPixelOfScreen (screen);
	}
      else if (! XAllocColor (dpy, cmap, fg_color))
	{
	  fprintf (stderr,
		   "%s: couldn't allocate color \"%s\", using black\n",
		   progname, fg_name);
	  fg_color->pixel = BlackPixelOfScreen (screen);
	}

      if (! XParseColor (dpy, cmap, bg_name, bg_color))
	{
	  fprintf (stderr, "%s: can't parse color %s; using white\n",
		   progname, bg_name);
	  bg_color->pixel = WhitePixelOfScreen (screen);
	}
      else if (! XAllocColor (dpy, cmap, bg_color))
	{
	  fprintf (stderr,
		   "%s: couldn't allocate color \"%s\", using white\n",
		   progname, bg_name);
	  bg_color->pixel = WhitePixelOfScreen (screen);
	}

      /* kludge -rv */
      if (get_boolean_resource ("reverseVideo", "ReverseVideo"))
	{
	  XColor swap;
	  swap = *fg_color;
	  *fg_color = *bg_color;
	  *bg_color = swap;
	}
    }

  if (! bd_name)
    bd_name = fg_name;

  /* Set border color to something reasonable in the colormap */
  if (! XParseColor (dpy, cmap, bd_name, bd_color))
    {
      fprintf (stderr, "%s: can't parse color %s; using white\n",
	       progname, bd_name);
      bd_color->pixel = WhitePixelOfScreen (screen);
    }
  else if (! XAllocColor (dpy, cmap, bd_color))
    {
      fprintf (stderr, "%s: couldn't allocate color \"%s\", using white\n",
	       progname, bd_name);
      bd_color->pixel = WhitePixelOfScreen (screen);
    }
}

void
cycle_colors (dpy, cmap, fg_color, bg_color, window, fg_gc, bg_gc)
     Display *dpy;
     Colormap cmap;
     XColor *fg_color, *bg_color;
     Window window;
     GC fg_gc, bg_gc;
{
  static int hue_tick;
  hsv_to_rgb (hue_tick,
	      1.0, 1.0,
	      &fg_color->red, &fg_color->green, &fg_color->blue);
  hsv_to_rgb ((hue_tick + 180) % 360,
	      1.0, 1.0,
	      &bg_color->red, &bg_color->green, &bg_color->blue);
  hue_tick = (hue_tick+1) % 360;

  if (! no_writable_cells)
    {
      XStoreColor (dpy, cmap, fg_color);
      XStoreColor (dpy, cmap, bg_color);
    }
  else
    {
      XColor tmp;
      fg_color->flags = bg_color->flags = DoRed | DoGreen | DoBlue;

      tmp = *fg_color;
      if (!XAllocColor(dpy, cmap, fg_color))
	*fg_color = tmp;
      else
	{
	  XFreeColors (dpy, cmap, &tmp.pixel, 1, 0);
	  fg_color->red   = tmp.red;
	  fg_color->green = tmp.green;
	  fg_color->blue  = tmp.blue;
	}

      tmp = *bg_color;
      if (!XAllocColor(dpy, cmap, bg_color))
	*bg_color = tmp;
      else
	{
	  XFreeColors (dpy, cmap, &tmp.pixel, 1, 0);
	  bg_color->red   = tmp.red;
	  bg_color->green = tmp.green;
	  bg_color->blue  = tmp.blue;
	}

      XSetForeground (dpy, fg_gc, fg_color->pixel);
      XSetBackground (dpy, fg_gc, bg_color->pixel);
      XSetForeground (dpy, bg_gc, bg_color->pixel);
      XSetBackground (dpy, bg_gc, fg_color->pixel);
      XSetWindowBackground (dpy, window,
#ifdef SHAPE
			    do_shape ? fg_color->pixel :
#endif
			    bg_color->pixel);
    }
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: colors.c,v $ $Date: 2003/04/15 20:48:47 $ $Revision: 1.1.1.1 $ $Name:  $"};
