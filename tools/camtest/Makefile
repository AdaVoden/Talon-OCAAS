CLDFLAGS =
CFLAGS = $(CLDFLAGS) -O2 -Wall -I../../libfits -I../../libmisc -I../../libastro
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lfits -lmisc -lastro -lm

camtest: camtest.o
	$(CC) $(LDFLAGS) -o camtest camtest.o $(LIBS)

camtesthi: camtesthi.o
	$(CC) $(LDFLAGS) -o camtesthi camtesthi.o $(LIBS)

install: camtest camtesthi
	rm -f $(TELHOME)/bin/camtest
	cp camtest $(TELHOME)/bin/camtest
	rm -f $(TELHOME)/bin/camtesthi
	cp camtesthi $(TELHOME)/bin/camtesthi

pass8b:	install

clobber:
	touch x.o
	rm -fr *.o camtest camtesthi

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $
