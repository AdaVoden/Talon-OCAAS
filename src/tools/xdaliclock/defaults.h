/* Resource and command-line junk
 *
 * xdaliclock.c, Copyright (c) 1991, 1992 Jamie Zawinski <jwz@netscape.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */


static char *defaults[] = {
  "*name:		XDaliClock",
  "*mode:		24",
  "*datemode:		MM/DD/YY",
  "*seconds:		True",
  "*cycle:		False",
  "*shape:		True",
  "*memory:		Medium",
  "*title:		Dali Clock",
  "*foreground:		#fec500",
  "*background:		white",
  "XDaliClock*font:	9x15",
  0
};

/* The font resource above has the class specified so that if the user
   has "*font:" in their personal resource database, XDaliClock won't
   inherit it unless its class is specified explicitly.  To do otherwise
   would be annoying.
 */

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: defaults.h,v $ $Date: 2003/04/15 20:48:47 $ $Revision: 1.1.1.1 $ $Name:  $
 */
