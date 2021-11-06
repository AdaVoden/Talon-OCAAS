HDIR = ../../libmisc
MJR = 121

CFLAGS = -O -DPC39_MJR=$(MJR) -DMODULE -D__KERNEL__ -Wall -I$(HDIR)
NAME = pc39
DIR = $(TELHOME)/dev

pass8:	$(NAME).o
	@echo 'Now run "make install" as root'

pass8a:	install

install: $(NAME).o
	mkdir -p $(DIR)
	cp $(NAME).o $(DIR)/$(NAME).o
	chmod 660 $(DIR)/$(NAME).o
	rm -f $(DIR)/$(NAME)
	mknod $(DIR)/$(NAME) c $(MJR) 0
	chmod 660 $(DIR)/$(NAME)
	chown -R talon $(DIR)
	chgrp -R talon $(DIR)

clobber:
	touch x.o
	rm -f *.o

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $
