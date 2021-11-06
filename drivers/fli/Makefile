# Major device number
MJR = 254

# where to place the module and its special file
DIR = $(TELHOME)/dev
NAME = fli

# which library code to use
FLIDIR = `./linux-kernel`

# note: problem w/2.4 library code; use 2.2 library and 2.4 driver

pass2:
	cp $(FLIDIR)/driver/*.* ./driver
	cp 2.2/lib/*.* ./lib
	cp 2.2/lib/Makefile ./lib
	cd lib; make libfli.o
pass8: 

pass8a: install

install:
	mkdir -p $(DIR)
	rm -f $(DIR)/$(NAME)*
	cp $(FLIDIR)/driver/$(NAME).o $(DIR)/$(NAME).o
	rm -f $(DIR)/ccdcamera
	ln -s $(NAME) $(DIR)/ccdcamera
	mknod $(DIR)/$(NAME) c $(MJR) 0

clobber:
	touch x.o
	rm -f *.o

# For CVS Only -- Do Not Edit
# $id$
