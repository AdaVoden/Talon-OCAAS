char *version = "@(#)\
XDaliClock v2.07; \
Copyright (c) 1991,1992,1993,1994,1995,1996 Jamie Zawinski (jwz@netscape.com)";

/*   7 oct 91 (v1.00)	submitted for X11r5 contrib tape.
    26 oct 91 (v1.01)	fixed Expose handling in -noseconds mode;
			access() called with too few args;
			added resource for MM/DD/YY, DD/MM/YY, etc.
     3 jan 92 (v1.02)	merged in VMS support from Daniel C. Newman
			<dan@innosoft.com>.
    16 jan 92 (v1.03)	added more intelligent visual support.
			Made it not die on fonts without per-char info.
     4 jun 92 (v1.04)   more VMS support for resource database files, from
			Tony Kennedy <adk@scri.fsu.edu>.
    10 jun 92 (v1.05)   More from Tony Kennedy: support visuals with different
			depths from default, center digits correctly in
			initial window, and merge geometry defaults in a more
			sophisticated way.  Merged in a slightly reworked
			version of more general segment-handling code from
			Dan Wallach <c169-bg@auriga.berkeley.edu>.  Added a
			second, even bigger builtin font.  Added the -root
			and -fullscreen arguments.
     3 feb 93 (v1.06)	Fixed some memory errors.
     4 sep 93 (v1.07)	Fixed shape handling; some minor portability changes.
    16 may 94 (v2.00)	Converted to use Xt instead of raw Xlib.
    21-May-94 (v2.01)	VMS and R3 support, with help from Richard L. Dyson
			<dyson@sunfish.physics.uiowa.edu>.
    31-May-94 (v2.03)	Minor tweaks to Imakefile; submitted for X11r6 contrib.
     8-Nov-94 (v2.04)	Some fixes for SGIs and non-default visuals.
     8-Jan-95 (v2.05)	Fixed a silly bug in determining the visual and cmap
			to use in -root mode, which caused it to not work with
			xscreensaver when the saver wasn't using the default
			visual and cmap.
    24-Dec-95 (v2.06)	Made -cycle work on non-PseudoColor visuals.
			Fixed a couple of shape-related bugs.
    23-Feb-96 (v2.07)	Added WM_COMMAND property for session management.
			More better VMS support from Martin Zinser.
 */
