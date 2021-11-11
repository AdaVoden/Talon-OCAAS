/* this program can be run as "ricohmnt", "ricohumnt", "ricohfsck" or
 *    "ricohfmt".
 * IFF it is suid root.
 */

#include <stdio.h>
#include <libgen.h>

main (ac, av)
int ac;
char *av[];
{
	char *base;

	if (geteuid() != 0) {
	    fprintf (stderr, "Sorry\n");
	    exit (1);
	}
	if (ac != 1) {
	    fprintf (stderr, "No args\n");
	    exit (1);
	}

	base = basenm(av[0]);
	if (strcmp (base, "ricohmnt") == 0)
	    execlp ("/sbin/mount", "mount", "-F", "vxfs", "/dev/dsk/c1t0d0s1",
						    "/ricoh", (char *)0);
	else if (strcmp (base, "ricohumnt") == 0)
	    execlp ("/sbin/umount", "umount", "/ricoh", (char *)0);
	else if (strcmp (base, "ricohfsck") == 0)
	    execlp ("/sbin/fsck", "fsck", "-F", "vxfs", "/dev/dsk/c1t0d0s1",
								(char *)0);
	else if (strcmp (base, "ricohfmt") == 0)
	    execlp ("/etc/diskadd", "diskadd", "c1t0d0", (char *)0);
	fprintf (stderr, "Can not exec %s\n", av[0]);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: ricohmnt.c,v $ $Date: 2003/04/15 20:48:39 $ $Revision: 1.1.1.1 $ $Name:  $"};
