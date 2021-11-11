CLDFLAGS= 
CFLAGS= $(CLDFLAGS) -O2 -Wall -I../../libastro -I../../libfits -I../../libmisc
LDFLAGS= $(CLDFLAGS) -L$(TELHOME)/lib
LIBS= -lastro -lmisc -lfits -lm

OBJSW = fitswindow.o

fitswindow: $(OBJSW)
	$(CC) $(LDFLAGS) -o $@ $(OBJSW) $(LIBS)

pass5: install

install: fitswindow fitstogif fitsindex
	rm -f $(TELHOME)/bin/fitswindow
	cp fitswindow $(TELHOME)/bin/fitswindow
	rm -f $(TELHOME)/bin/fitsindex
	cp fitsindex $(TELHOME)/bin/fitsindex
	rm -f $(TELHOME)/bin/fitstogif
	cp fitstogif $(TELHOME)/bin/fitstogif

clobber:
	touch x.o
	rm -fr *.o fitswindow

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:33 $ $Revision: 1.1.1.1 $ $Name:  $
