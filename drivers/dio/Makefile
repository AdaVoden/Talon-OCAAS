HDIR = ../../libmisc
MJR = 122

CFLAGS = -O -DDIO_MJR=$(MJR) -DMODULE -D__KERNEL__ -Wall -I$(HDIR)
NAME = dio
DIR = $(TELHOME)/dev

$(NAME).o: $(HDIR)/$(NAME).h $(NAME).c

install: $(NAME).o
	mkdir -p $(DIR)
	cp $(NAME).o $(DIR)/$(NAME).o
	chmod 660 $(DIR)/$(NAME).o
	rm -f $(DIR)/$(NAME)96
	mknod $(DIR)/$(NAME)96 c $(MJR) 0
	chmod 660 $(DIR)/$(NAME)96
	# rm -f $(DIR)/$(NAME)24
	# mknod $(DIR)/$(NAME)24 c $(MJR) 1
	# chmod 660 $(DIR)/$(NAME)24
	chown -R talon $(DIR)
	chgrp -R talon $(DIR)

pass8atf:	$(NAME).o

pass8aatf: install

clobber:
	touch x.o
	rm -f *.o $(DIR)/$(NAME).o

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:05 $ $Revision: 1.1.1.1 $ $Name:  $
