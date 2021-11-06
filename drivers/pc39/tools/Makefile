CLDFLAGS = 
CFLAGS = $(CLDFLAGS) -O2 -Wall -I../../../libastro -I../../../libmisc
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lmisc -lastro -lfits -lm

all: pc39 pc39cmd pc39stat logaxis logencoders

install: pc39 pc39cmd pc39stat logaxis logencoders
	cp pc39 $(TELHOME)/bin
	cp pc39cmd $(TELHOME)/bin
	cp pc39stat $(TELHOME)/bin
	cp logaxis $(TELHOME)/bin
	cp logencoders $(TELHOME)/bin
	cp autodiff $(TELHOME)/bin
	chmod u+w $(TELHOME)/bin/autodiff

pass8b: install

pc39:	pc39.o
	$(CC) $(LDFLAGS) -o pc39 pc39.o $(LIBS)

pc39cmd:	pc39cmd.o
	$(CC) $(LDFLAGS) -o pc39cmd pc39cmd.o $(LIBS)

pc39stat:	pc39stat.o
	$(CC) $(LDFLAGS) -o pc39stat pc39stat.o $(LIBS)

logaxis:	logaxis.o
	$(CC) $(LDFLAGS) -o logaxis logaxis.o -lm $(LIBS)

logencoders:	logencoders.o
	$(CC) $(LDFLAGS) -o logencoders logencoders.o -lm $(LIBS)

clobber:
	touch x.o
	rm -f *.o pc39 pc39cmd pc39stat logaxis logencoders

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:10 $ $Revision: 1.1.1.1 $ $Name:  $
