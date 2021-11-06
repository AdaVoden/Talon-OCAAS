/* space.c file for the CyberResearch, Inc, CIO-DIO 8255 I/O.
 */

/* short-lived file in /etc/conf/cf.d during system builds */
#include <config.h>

int cio_n = CIO_CNTLS;

int cio_io_base[] = {
#ifdef CIO_0
    CIO_0_SIOA
#endif
#ifdef CIO_1
    CIO_1_SIOA
#endif
};

int cio_io_end[] = {
#ifdef CIO_0
    CIO_0_EIOA
#endif
#ifdef CIO_1
    CIO_1_EIOA
#endif
};

int cio_io_type[] = {
#ifdef CIO_0
    CIO_0
#endif
#ifdef CIO_1
    CIO_1
#endif
};

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: space.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
