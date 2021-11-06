# Makefile for fcompress and fdecompress

IS = \
	-I../../../../libastro \
	-I../../../../libmisc \
	-I../../../../libfits

CLDFLAGS=
CFLAGS = $(CLDFLAGS) -O2 $(IS)
LDFLAGS = $(CLDFLAGS) -L$(TELHOME)/lib
LIBS = -lastro -lmisc -lfits -lwcs -lfs -lm

all:	fcompress fdecompress

fcompress: fcompress.o
	$(CC) $(LDFLAGS) -o fcompress fcompress.o $(LIBS)

fdecompress: fdecompress.o
	$(CC) $(LDFLAGS) -o fdecompress fdecompress.o $(LIBS)

pass5:	install

install: fcompress fdecompress
	rm -f $(TELHOME)/bin/fcompress
	cp fcompress $(TELHOME)/bin/fcompress
	rm -f $(TELHOME)/bin/fdecompress
	cp fdecompress $(TELHOME)/bin/fdecompress

clobber:
	touch x.o
	rm -f *.o fcompress fdecompress

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $
