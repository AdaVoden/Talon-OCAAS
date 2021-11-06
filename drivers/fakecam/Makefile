# Major device number
MJR = 120

# where to place the module and its special file
DIR = $(TELHOME)/dev

MDIR = ../../libmisc
CFLAGS = -O -DAP_MJR=$(MJR) -DMODULE -D__KERNEL__ -Wall -I$(MDIR)
NAME = fakecam

OBJS = main.o aplinux.o apdata.o

$(NAME).o: apccd.h apglob.h apdata.h aplinux.h aplow.h $(OBJS)
	ld -r -o $(NAME).o $(OBJS)

install: $(NAME).o
	mkdir -p $(DIR)
	rm -f $(DIR)/$(NAME)*
	cp $(NAME).o $(DIR)/$(NAME).o
	chmod 660 $(DIR)/$(NAME).o
	mknod $(DIR)/$(NAME) c $(MJR) 0
	chmod 660 $(DIR)/$(NAME)
	rm -f $(DIR)/ccdcamera
	ln -s $(NAME) $(DIR)/ccdcamera
	chown -R talon $(DIR)
	chgrp -R talon $(DIR)

clobber:
	touch x.o
	rm -f *.o $(DIR)/$(NAME)*

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $
