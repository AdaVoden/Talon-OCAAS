/* simple program to open the pc39 and read the current status regs.
 * N.B. reading some bits also clears them.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "telenv.h"
#include "pc39.h"

char dev[] = "dev/pc39";

int
main (int ac, char *av[])
{
	int pc39;
	PC39_Regs regs;

	pc39 = telopen (dev, O_RDWR);
	if (pc39 < 0) {
	    perror (dev);
	    exit(1);
	}

	if (ioctl (pc39, PC39_GETREGS, &regs) < 0) {
	    perror ("GETREGS");
	    exit (1);
	}

	printf ("Done = 0x%02x\n", regs.done);
	    if (regs.done & PC39_DONE_X) printf ("  X\n");
	    if (regs.done & PC39_DONE_Y) printf ("  Y\n");
	    if (regs.done & PC39_DONE_Z) printf ("  Z\n");
	    if (regs.done & PC39_DONE_T) printf ("  T\n");
	    if (regs.done & PC39_DONE_U) printf ("  U\n");
	    if (regs.done & PC39_DONE_V) printf ("  V\n");
	    if (regs.done & PC39_DONE_R) printf ("  R\n");
	    if (regs.done & PC39_DONE_S) printf ("  S\n");
	printf ("Ctrl = 0x%02x\n", regs.ctrl);
	    if (regs.ctrl & PC39_CTRL_DMA_E) printf ("  DMA_E\n");
	    if (regs.ctrl & PC39_CTRL_DMA_DIR) printf ("  DMA_DIR\n");
	    if (regs.ctrl & PC39_CTRL_DON_E) printf ("  DON_E\n");
	    if (regs.ctrl & PC39_CTRL_IBF_E) printf ("  IBF_E\n");
	    if (regs.ctrl & PC39_CTRL_TBE_E) printf ("  TBE_E\n");
	    if (regs.ctrl & PC39_CTRL_IRQ_E) printf ("  IRQ_E\n");
	printf ("Stat = 0x%02x\n", regs.stat);
	    if (regs.stat & PC39_STAT_CMD_S) printf ("  CMD_S\n");
	    if (regs.stat & PC39_STAT_INIT)  printf ("  INIT\n");
	    if (regs.stat & PC39_STAT_ENC_S) printf ("  ENC_S\n");
	    if (regs.stat & PC39_STAT_OVRT)  printf ("  OVRT\n");
	    if (regs.stat & PC39_STAT_DON_S) printf ("  DON_S\n");
	    if (regs.stat & PC39_STAT_IBF_S) printf ("  IBF_S\n");
	    if (regs.stat & PC39_STAT_TBE_S) printf ("  TBE_S\n");
	    if (regs.stat & PC39_STAT_IRQ_S) printf ("  IRQ_S\n");

	(void) close (pc39);

	return (0);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = {(char *)rcsid, "@(#) $RCSfile: pc39stat.c,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $"};
