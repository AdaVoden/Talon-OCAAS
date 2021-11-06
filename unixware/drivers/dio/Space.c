/* space.c file for the CyberResearch, Inc, CIO-DIO 8255 I/O.
 */

/* short-lived file in /etc/conf/cf.d during system builds */
#include <sys/param.h>
#include <sys/types.h>

#include "config.h"

#include <sys/dio.h>

int dio_n = DIO_CNTLS;

/* define the dio_info array.
 * there is one array per potential board.
 * the actual number depends on the number of lines in System.
 * this is enough for three boards; just follow the pattern to add more.
 * N.B. remember, this just allows for this many; you still need to edit
 *   System and Node to actually build for so many boards.
 */
DIOInfo dio_info[] = {
#ifdef DIO_0
    {
	DIO_0_SIOA,
	DIO_0_EIOA,
	DIO_0
    },
#endif
#ifdef DIO_1
    {
	DIO_1_SIOA,
	DIO_1_EIOA,
	DIO_1
    },
#endif
#ifdef DIO_2
    {
	DIO_2_SIOA,
	DIO_2_EIOA,
	DIO_2
    },
#endif
};

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: Space.c,v $ $Date: 2003/04/15 20:48:48 $ $Revision: 1.1.1.1 $ $Name:  $"};
