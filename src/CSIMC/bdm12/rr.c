#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#define	SPEED	B38400		/* desired speed */

int
main (int ac, char *av[])
{
	int arg;

	/* set raw character access */
	if (ac > 1) {
	    struct termios tio;
	    memset (&tio, 0, sizeof(tio));
	    tio.c_cflag = CS8|CREAD|CLOCAL;
	    tio.c_cc[VMIN] = 1;
	    tio.c_cc[VTIME] = 0;
	    cfsetospeed (&tio, SPEED);
	    cfsetispeed (&tio, SPEED);
	    if (tcsetattr (0, TCSANOW, &tio) < 0) {
		perror ("");
		exit (1);
	    }
	}

	/* RTS off to receive */
	arg = TIOCM_DTR; ioctl (0, TIOCMSET, &arg);
	arg = TIOCM_RTS; ioctl (0, TIOCMSET, &arg);
	arg = TIOCM_DTR; ioctl (0, TIOCMSET, &arg);

	/* read and echo */
	while (1) {
	    char buf[100];
	    int n = read (0, buf, sizeof(buf));
	    int i;

	    for (i = 0; i < n; i++)
		write (1, buf+i, 1);
	}
	
	return (0);
}
