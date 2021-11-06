/* see start.c for explanation */

	asm (".area text");
	asm ("__text_end::");
	asm (".area data");
	asm ("__data_end::");
	asm (".area bss");
	asm ("__bss_end::");

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: end.c,v $ $Date: 2003/04/15 20:46:48 $ $Revision: 1.1.1.1 $ $Name:  $
 */
