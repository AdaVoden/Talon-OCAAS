HDIR = ../../libmisc
MJR = 124

CFLAGS = -O -DPDL_MJR=$(MJR) -DMODULE -D__KERNEL__ -Wall -I$(HDIR)
NAME = paddle
DIR = $(TELHOME)/dev

xpass8:	$(NAME).o
	@echo 'Now run "make install" as root'

xpass8a:	install

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
	rm -f *.o $(DIR)/$(NAME).o

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $
