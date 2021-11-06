MDIR = ../../libmisc
MJR = 123

CFLAGS = -O -DHPC_MJR=$(MJR) -DMODULE -D__KERNEL__ -Wall -I$(MDIR)
NAME = hpc
DIR = $(TELHOME)/dev

OBJS = main.o hpc1.o

$(NAME).o: hpc.h $(OBJS)
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

pass8atf:	$(NAME).o

clobber:
	touch x.o
	rm -f *.o

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:09 $ $Revision: 1.1.1.1 $ $Name:  $
